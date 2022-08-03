#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <getopt.h>
#include "config.h"
#include "../spm.h"
#include "spm_xdma.h"
#include "../../../protocol/resetful/data_frame.h"
#include "../../../bsp/io.h"
#include "../agc/agc.h"
#include "../statistics/statistics.h"



static int xspm_read_stream_stop(int ch, int subch, enum stream_type type);
static int xspm_read_xdma_data_over(int ch,  void *arg,  int type);
static int xspm_xdma_data_clear(int ch,  void *arg, int type);

#define DEFAULT_IQ_SEND_BYTE 512
size_t iq_send_unit_byte = DEFAULT_IQ_SEND_BYTE;    /* IQ发送长度 */

static inline void *zalloc(size_t size)
{
	return calloc(1, size);
}

static void print_array(uint8_t *ptr, ssize_t len)
{
    if(ptr == NULL || len <= 0)
        return;
    
    for(int i = 0; i< len; i++){
        if(i % 16 == 0 && i != 0)
            printf("\n");
        printf("%02x ", *ptr++);
    }
    printf("\n----------------------\n");
}

static void _spm_gettime(struct timeval *tv)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    tv->tv_sec = ts.tv_sec;
    tv->tv_usec = ts.tv_nsec / 1000;
}

static int _spm_tv_diff(struct timeval *t1, struct timeval *t2)
{
    return
        (t1->tv_sec - t2->tv_sec) * 1000 +
        (t1->tv_usec - t2->tv_usec) / 1000;
}

int _WaitDeviceReady(int dev_fd)
{
    #define DEVICE_READY_TIMEOUT 10000  //unit is ms
	int ret = 0;
	int isReady = 0;
	int timeout = DEVICE_READY_TIMEOUT;
	enum xdma_status status;
	struct xdma_ring_trans_ioctl ring_trans;
	memset(&ring_trans, 0, sizeof(struct xdma_ring_trans_ioctl));

	while(timeout--)
	{
		ret = ioctl(dev_fd, IOCTL_XDMA_STATUS, &status);
		if (ret) 
		{
			printf("ioctl(IOCTL_XDMA_STATUS) failed %d, status:%d\n", ret, status);
			return -1;	  
		}	

		printf_debug("IOCTL_XDMA_STATUS:%d\n", status);
		if(status == XDMA_STATUS_BUSY)
		{
			isReady = 1;
			break;
		}
		
		usleep(1000);
	}

	if(isReady)
		return 1;
		
	return 0;   // time out 
}

static int xspm_create(void)
{
    struct _spm_xstream *pstream;
    struct xdma_ring_trans_ioctl ring_trans;
    int i = 0, rc;
    int pagesize = getpagesize();
    int dev_len = 0;
    printf_info("SPM init\n");
#ifdef DEBUG_TEST
    return 0;
#endif
    pstream = spm_dev_get_stream(&dev_len);
    /* create stream */
    for(i = 0; i< dev_len ; i++){
        pstream[i].base.id = open(pstream[i].base.devname, O_RDWR);
        if( pstream[i].base.id < 0){
            fprintf(stderr, "[%d]open:%s, %s\n", i, pstream[i].base.devname, strerror(errno));
            continue;
        }
        printf_info("mmap: %s\n", pstream[i].base.devname);
        memset(&ring_trans, 0, sizeof(struct xdma_ring_trans_ioctl));
        ring_trans.block_size = pstream[i].block_size;
        ring_trans.block_count = pstream[i].base.len / pstream[i].block_size;
        printf_info("block_size=%u, block_count=%u, len=%u\n", ring_trans.block_size, ring_trans.block_count, pstream[i].base.len);
        xspm_read_stream_stop(pstream[i].base.ch, -1, pstream[i].base.type);
        if(pstream[i].base.rd_wr == DMA_READ){
            rc = ioctl(pstream[i].base.id, IOCTL_XDMA_INIT_BUFF, &ring_trans);  //close时释放
            if (rc == 0) {
                printf_info("IOCTL_XDMA_INIT_BUFF succesful.\n");
            } 
            else {
                printf("ioctl(IOCTL_XDMA_INIT_BUFF) failed= %d\n", rc);
                exit(-1);
            }
            printf_note("[%d, ch=%d]create stream[%s] dev:%s len=%u, block_size:%u, block_count=%u\n", pstream[i].base.id,pstream[i].base.ch, pstream[i].base.name, 
                                pstream[i].base.devname, pstream[i].base.len, pstream[i].block_size, ring_trans.block_count);
            for(int j = 0; j < ring_trans.block_count; j++){
                pstream[i].ptr_vec[j] = mmap(NULL, ring_trans.block_size, PROT_READ | PROT_WRITE,MAP_SHARED, pstream[i].base.id, j * pagesize);
                if (pstream[i].ptr_vec[j] == (void*) -1) {
                    fprintf(stderr, "mmap: %s\n", strerror(errno));
                    exit(-1);
                }
                printf_info("block[%d]: ptr=%p, pagesize=%d\n", j, pstream[i].ptr_vec[j], pagesize);
            }
        }
        usleep(1000);
    }
    
    return 0;
}


struct xdma_ring_trans_ioctl xinfo[4];
static ssize_t xspm_stream_read(int ch, int index, int type,  void **data, uint32_t *len, void *args)
{
    #define _STREAM_READ_TIMEOUT_MS (1000)
    int rc = 0;
    struct _spm_xstream *pstream = spm_dev_get_stream(NULL);
    struct xdma_ring_trans_ioctl *info = &xinfo[index];
    size_t readn = 0;
    static uint32_t timer = 0;
    struct timeval start, now;
    
    memset(info, 0, sizeof(struct xdma_ring_trans_ioctl));
    if(pstream[index].base.id < 0){
        printf_note("%d stream node:%s not found\n",index, pstream[index].base.name);
        return -1;
    }
     _spm_gettime(&start);
    do{
        rc =  ioctl(pstream[index].base.id, IOCTL_XDMA_TRANS_GET, info);
        if (rc) {
            printf_err("type=%d, id=%d ioctl(IOCTL_XDMA_TRANS_GET) failed %d, info=%p, %p, %p\n",index, pstream[index].base.id, rc, info, &xinfo[0], &xinfo[1]);
            return -1;
        }
        if(info->status == RING_TRANS_OK){
            printf_debug("type=%d\n", index);
            printf_debug("ring_trans.rx_index:%d\n", info->rx_index);
            printf_debug("ring_trans.ready_count:%d\n", info->ready_count);
            printf_debug("ring_trans.status:%d\n", info->status);
        } else if(info->status == RING_TRANS_FAILED){
            printf_err("status:RING_TRANS_FAILED.\n");
            return -1;
        } else if(info->status == RING_TRANS_OVERRUN){
            #ifdef CONFIG_SPM_STATISTICS
            update_dma_over_run_count(index, 1);
            #endif
            printf_warn("*****status:RING_TRANS_OVERRUN.*****\n");
            xspm_xdma_data_clear(ch, args, index);
        } else if(info->status == RING_TRANS_INITIALIZING){
            printf_warn("*****status:RING_TRANS_INITIALIZING.*****\n");
            usleep(10);
        } else if(info->status == RING_TRANS_PENDING){
            printf_debug("*****status:RING_TRANS_PENDING\n");
            usleep(1);
        }
        _spm_gettime(&now);
        if(_spm_tv_diff(&now, &start) > _STREAM_READ_TIMEOUT_MS){
            printfn("Read TimeOut![%u]\r",timer++);
            break;
        }
    }while(info->status == RING_TRANS_PENDING);

    int j;
    uint8_t *ptr = NULL;
    if(info->ready_count > 0)
        printf_note("ready_count: %u, type:%d\n", info->ready_count, type);
    for(int i = 0; i < info->ready_count; i++){
        j = (info->rx_index + i) % info->block_count;
        data[i] = pstream[index].ptr_vec[j];
        len[i] = info->results[j].length;
        timer = 0;
        printf_note("[%d,index:%d][%p, %p, len:%u, offset=0x%x]%s\n", 
                i, j, data[i], pstream[index].ptr_vec[j], len[i], info->rx_index,  pstream[index].base.name);
#ifdef CONFIG_FILE_SINK
        int sink_type = FILE_SINK_TYPE_FFT;
        if(type == STREAM_NIQ)
            sink_type = FILE_SINK_TYPE_NIQ;
        else if(type == STREAM_BIQ)
            sink_type = FILE_SINK_TYPE_BIQ;
        else if(type == STREAM_XDMA_READ)
            sink_type = FILE_SINK_TYPE_RAW;
        file_sink_work(sink_type, data[i], len[i]);
#endif
    }
#ifdef CONFIG_SPM_STATISTICS
        struct spm_run_parm *r = args;
        if(r)
            r->dma_ch = pstream[index].base.dma_ch;

        for(int i = 0; i < info->ready_count; i++)
            update_dma_readbytes(index, len[i]);
        update_dma_read_pkts(index, info->ready_count);
#endif
    return info->ready_count;
}



static inline int xspm_find_index_by_type(int ch, int subch, enum stream_type type)
{
    int dev_len = 0;
    struct _spm_xstream *pstream = spm_dev_get_stream(&dev_len);
    int i, find = 0, index = -1;

    subch = subch;
    for(i = 0; i < dev_len; i++){
        if((type == pstream[i].base.type) && (ch == pstream[i].base.ch || pstream[i].base.ch == -1)){
            index = i;
            find = 1;
            break;
        }
    }
    
    if(find == 0)
        return -1;
    
    return index;
}

static inline int xspm_find_index_by_rw(int ch, int subch, int rw)
{
    int dev_len = 0;
    struct _spm_xstream *pstream = spm_dev_get_stream(&dev_len);
    int i, find = 0, index = -1;

    subch = subch;
    for(i = 0; i < dev_len; i++){
        if(rw == pstream[i].base.rd_wr && (ch == pstream[i].base.ch || pstream[i].base.ch == -1)){
            index = i;
            find = 1;
            break;
        }
    }
    
    if(find == 0)
        return -1;
    
    return index;
}

static int xspm_stram_write(int ch, const void *data, size_t data_len)
{
    ssize_t ret = 0, len;
    int index, buflen = data_len;
    char *buf = data;
    struct _spm_xstream *pstream = spm_dev_get_stream(NULL);

    if (!buflen)
        return 0;

#ifdef DEBUG_TEST
    return data_len;
#endif
    index = xspm_find_index_by_type(ch, -1, STREAM_XDMA_WRITE);

    if(index < 0)
        return -1;
    
    if(pstream[index].base.id < 0)
        return -1;

    while (buflen) {
        len =  write(pstream[index].base.id, buf, buflen);
        if (len < 0) {
            printf_note("[fd:%d]-send len : %ld, %d[%s][%s], %d, %p\n", pstream[index].base.id, len, errno, strerror(errno), pstream[index].base.name, buflen, buf);
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOTCONN)
                break;
            return -1;
        }
        ret += len;
        buf += len;
        buflen -= len;
    }
#ifdef CONFIG_SPM_STATISTICS
    update_dma_write_bytes(index, ret);
#endif
    return ret;
}

static inline int _align_4byte(int *rc)
{
    int _rc = *rc;
    int reminder = _rc % 4;
    if(reminder != 0){
        _rc += (4 - reminder);
        *rc = _rc;
        return (4 - reminder);
    }
    return 0;
}
static int xspm_stram_write_align(int ch, const void *data, size_t len)
{
    ssize_t nwrite;
    void *buffer = NULL;
    int pagesize = 0,reminder = 0, _len = 0;

    pagesize=getpagesize();
    posix_memalign((void **)&buffer, pagesize /*alignment */ , len + pagesize);
    if (!buffer) {
        fprintf(stderr, "OOM %u.\n", pagesize);
        return -1;
    }
    memset(buffer, 0 , len + pagesize);
    memcpy(buffer, data, len);
    _len = len;
    reminder = _align_4byte((int *)&_len);
#ifndef DEBUG_TEST
    nwrite = xspm_stram_write(ch, buffer, _len);
#else
    nwrite = _len;
#endif
    printf_note("Write data %s![%ld],align 4byte reminder=%d[raw len: %lu]\n",  (nwrite == _len) ? "OK" : "Faild", nwrite, reminder, len);
    free(buffer);
    buffer = NULL;
    return nwrite;
}

static ssize_t _xspm_find_header(uint8_t *ptr, uint16_t header, size_t len)
{
    ssize_t offset = 0;
    do{
        if(ptr != NULL && *(uint16_t *)ptr != header){
            ptr += 2;
            offset += 2;
        }else{
            break;
        }
    }while(offset < len);

    if(offset >= len || ptr == NULL)
        return -1;

    return offset;
}


static ssize_t xspm_stream_read_from_file(int type, int ch, void **data, size_t *len, void *args)
{
    #define STRAM_IQ_FILE_PATH "/home/ycs/share/platform-2.0.0/files/platform-2.0.0/DEV0_CH0_IQ.raw"
    #define STRAM_FFT_FILE_PATH "/home/kylin/src/platform2.0.0/platform-2.0.0/fft512.dat"
    //#define STRAM_FFT_FILE_PATH "/home/ycs/share/platform-2.0.0/files/platform-2.0.0/DEV0_CH1_FFT8K.raw"
    #define STRAM_READ_BLOCK_SIZE  528
    #define STRAM_READ_BLOCK_COUNT 2

    size_t rc = 0, rc2 = 0;
    static FILE * fp = NULL;
    static void *buffer[STRAM_READ_BLOCK_COUNT] = {[0 ... STRAM_READ_BLOCK_COUNT-1] = NULL};
    char *path = NULL;
    
    if(type == STREAM_FFT){
        path = STRAM_FFT_FILE_PATH;
    }else
        path = STRAM_IQ_FILE_PATH;
    if(fp == NULL){
        fp = fopen (path, "rb");
        if(!fp){
            printf_err("Open file[%s] error!\n", path);
            return -1;
        }
    }

    for(int i = 0; i < STRAM_READ_BLOCK_COUNT; i++){
        if(buffer[i] == NULL){
            buffer[i] = safe_malloc(STRAM_READ_BLOCK_SIZE);
            if(!buffer[i]){
                printf_warn("Malloc faild\n");
                return -1;
            }
        }
    }

    uint32_t cn = 0;
    ssize_t offset = 0;
    uint8_t *ptr = NULL;
    do{
        rc2 = 0;
        rc = fread(buffer[cn], 1, STRAM_READ_BLOCK_SIZE, fp);
        if(rc == 0){ /* read over */
            printf_note("Read over!%d\n", feof(fp));
            rewind(fp);
            printf_note("ftell:%ld\n", ftell(fp));
            break;
        }
        ptr = buffer[cn];
        offset = _xspm_find_header(buffer[cn], 0xaa55, rc);
        if(offset < 0){ 
            printf_note("find header err\n");
            continue;
        }
        else{
            ptr += rc;
            if(offset > 0){
                printf_note("[%02x]rc=%lu, offset=%ld, %lu\n", *ptr, rc, offset, rc - offset);
                rc2 = fread((void *)ptr, 1, offset , fp);
            }
        }
        data[cn] = (uint8_t *)buffer[cn] + offset;
        len[cn] = rc - offset + rc2;

        //printf_note("cn:%u, data=%p, len:%lu,offset:%ld, rc=%lu, %lu, sn=%d\n", cn, data[cn], len[cn], offset, rc, rc2, *((uint8_t *)data[cn]+2));
        //print_array(data[cn], len[cn]);
        cn++;
    }while(cn < STRAM_READ_BLOCK_COUNT);
    //usleep(100);
    return cn;
}


ssize_t xspm_read_fft_vec_data(int ch , void **data, void *len, void *args)
{
#ifdef DEBUG_TEST
    if(config_get_work_enable() == false){
        usleep(1000);
        return -1;
    }
    return xspm_stream_read_from_file(STREAM_FFT, ch, data, len, args);
#else
    int index;
    
    index = xspm_find_index_by_type(-1, -1, STREAM_FFT);
    if(index < 0)
        return -1;
    return xspm_stream_read(ch, index, STREAM_FFT, data, len, args);
#endif
}

ssize_t xspm_read_iq_vec_data(int ch , void **data, void *len, void *args)
{
#ifdef DEBUG_TEST
    return xspm_stream_read_from_file(STREAM_NIQ, ch, data, len, args);
#else
    int index;
    
    index = xspm_find_index_by_type(-1, -1, STREAM_NIQ);
    if(index < 0)
        return -1;
    return xspm_stream_read(ch, index, STREAM_NIQ, data, len, args);
#endif
}


ssize_t xspm_read_niq_data(int ch , void **data, void *len, void *args)
{
    int index;
    
    index = xspm_find_index_by_type(-1, -1, STREAM_NIQ);
    if(index < 0)
        return -1;
    return xspm_stream_read(ch, index, STREAM_NIQ, data, len, args);
}


ssize_t xspm_read_biq_data(int ch , void **data, void *len, void *args)
{
    int index;
    
    index = xspm_find_index_by_type(-1, -1, STREAM_BIQ);
    if(index < 0)
        return -1;
    return xspm_stream_read(ch, index, STREAM_BIQ, data, len, args);

}

static ssize_t xspm_read_xdma_raw_data(int ch , void **data, void *len, void *args)
{
#ifdef DEBUG_TEST
       /* if(config_get_work_enable() == false){
            usleep(1000);
            return -1;
        }*/
        return xspm_stream_read_from_file(STREAM_FFT, ch, data, len, args);
#else
    int index = xspm_find_index_by_type(ch, -1, STREAM_XDMA_READ);
    if(index < 0)
        return -1;
    return xspm_stream_read(ch, index, STREAM_XDMA_READ, data, len, args);
#endif
}

static int xspm_read_xdma_raw_data_over(int ch,  void *arg)
{
    return xspm_read_xdma_data_over(ch, arg, STREAM_XDMA_READ);
}

static int xspm_send_data_by_fd(int fd, void *data, size_t len, void *arg)
{
    return tcp_send_data_to_client(fd, data, len);
}


static ssize_t xspm_read_fft_data(int ch, void **data, void *args)
{
    struct spm_run_parm *run_args = args;
    ssize_t fft_byte_len = 0;
#ifdef CONFIG_SPM_DISTRIBUTOR
    size_t byte_len = 0;
    byte_len = run_args->fft_size * sizeof(fft_t);
    if(spm_distributor_fft_data_frame_producer(ch, data, byte_len) == -1)
        return -1;
    fft_byte_len = byte_len;
#else
    ssize_t count = 0;
    int index;
    uint32_t  len[XDMA_TRANSFER_MAX_DESC] = {0};
    
    index = xspm_find_index_by_type(ch, -1, STREAM_FFT);
    if(index < 0)
        return -1;
    count = xspm_stream_read(ch, index, STREAM_FFT, data, len, args);
    count = count;
    fft_byte_len = len[0];
#endif
    return fft_byte_len;
}

static int xspm_send_fft_data(void *data, size_t fft_len, void *arg)
{
    #define HEAD_BUFFER_LEN  512 
    uint8_t *ptr_header = NULL;
    uint32_t header_len = 0;
    size_t data_byte_size = 0;

    if(data == NULL || fft_len == 0 || arg == NULL)
        return -1;
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;

    data_byte_size = fft_len* sizeof(fft_t);
#if (defined CONFIG_PROTOCOL_ACT)
    hparam->data_len = data_byte_size;
    hparam->sample_rate = io_get_raw_sample_rate(0, 0, hparam->scan_bw);
    hparam->type = SPECTRUM_DATUM_FLOAT;
    hparam->ex_type = SPECTRUM_DATUM;
    if((ptr_header = akt_assamble_data_frame_header_data(&header_len, arg)) == NULL){
        return -1;
    }
#elif defined(CONFIG_PROTOCOL_XW)
    hparam->data_len = data_byte_size;
    #if defined(CONFIG_SPM_FFT_EXTRACT_POINT)
    hparam->type = DEFH_DTYPE_CHAR;
    #else
    hparam->type = DEFH_DTYPE_SHORT;
    #endif
    hparam->ex_type = DFH_EX_TYPE_PSD;
    ptr_header = xw_assamble_frame_data(&header_len, arg);
    if(ptr_header == NULL)
        return -1;
#endif
    struct iovec iov[2];
    iov[0].iov_base = ptr_header;
    iov[0].iov_len = header_len;
    iov[1].iov_base = data;
    iov[1].iov_len = data_byte_size;
    int ch = hparam->ch;
    __lock_fft_send__(ch);
#if (defined CONFIG_PROTOCOL_DATA_TCP)
    tcp_send_vec_data(iov, 2, NET_DATA_TYPE_FFT);
#else
    udp_send_vec_data(iov, 2, NET_DATA_TYPE_FFT);
#endif
    __unlock_fft_send__(ch);
    _safe_free_(ptr_header);
    return (header_len + data_byte_size);
}

static void *_assamble_iq_header(int ch, size_t subch, size_t *hlen, size_t data_len, void *arg, enum stream_iq_type type)
{
    uint8_t *ptr = NULL, *ptr_header = NULL;
    uint32_t header_len = 0;
    struct poal_config *poal_config = &(config_get_config()->oal_config);
    struct sub_channel_freq_para_st *sub_channel_array;
    
    if(data_len == 0 || arg == NULL)
        return NULL;

    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
#ifdef CONFIG_PROTOCOL_ACT
    hparam->data_len = data_len;
    hparam->type = BASEBAND_DATUM_IQ;
    hparam->ex_type = DEMODULATE_DATUM;
    ch = hparam->ch;
    if(type == STREAM_NIQ_TYPE_AUDIO){
        int i;
        struct multi_freq_point_para_st  *points = &poal_config->channel[ch].multi_freq_point_param;
        i = executor_get_audio_point(ch);
        hparam->sub_ch_para.bandwidth_hz = points->points[i].d_bandwith;
        hparam->sub_ch_para.m_freq_hz =  points->points[i].center_freq;
        hparam->sub_ch_para.d_method = points->points[i].raw_d_method;
        hparam->sub_ch_para.sample_rate = 32000;    /* audio 32Khz */
    }else if(type == STREAM_NIQ_TYPE_RAW){
        sub_channel_array = &poal_config->channel[ch].sub_channel_para;
        hparam->sub_ch_para.bandwidth_hz = sub_channel_array->sub_ch[subch].d_bandwith;
        hparam->sub_ch_para.m_freq_hz = sub_channel_array->sub_ch[subch].center_freq;
        hparam->sub_ch_para.d_method = sub_channel_array->sub_ch[subch].raw_d_method;
        hparam->sub_ch_para.sample_rate =  io_get_raw_sample_rate(0, 0, hparam->sub_ch_para.bandwidth_hz);
    }
   
    printf_debug("ch=%d, subch = %zd, m_freq_hz=%"PRIu64", bandwidth:%uhz, sample_rate=%u, d_method=%d\n", 
        ch,
        subch,
        hparam->sub_ch_para.m_freq_hz,
        hparam->sub_ch_para.bandwidth_hz, 
        hparam->sub_ch_para.sample_rate,  
        hparam->sub_ch_para.d_method );
    
    if((ptr_header = akt_assamble_data_frame_header_data(&header_len, arg))== NULL){
        return NULL;
    }
    
#elif defined(CONFIG_PROTOCOL_XW)
    //ch = subch;
    hparam->data_len = data_len; 
    if(type == STREAM_BIQ_TYPE_RAW){
        printf_debug("ch=%d,middle_freq=%"PRIu64",bandwidth=%"PRIu64"\n",ch,poal_config->channel[ch].multi_freq_point_param.ddc.middle_freq,
                        poal_config->channel[ch].multi_freq_point_param.ddc.bandwidth);
        hparam->type = DEFH_DTYPE_BB_IQ;
        hparam->ddc_m_freq = poal_config->channel[ch].multi_freq_point_param.ddc.middle_freq;
        hparam->ddc_bandwidth = poal_config->channel[ch].multi_freq_point_param.ddc.bandwidth;
    }
    else if(type == STREAM_NIQ_TYPE_RAW || STREAM_NIQ_TYPE_AUDIO){
        sub_channel_array = &poal_config->channel[ch].sub_channel_para;
        hparam->ddc_bandwidth = sub_channel_array->sub_ch[subch].d_bandwith;
        hparam->ddc_m_freq = sub_channel_array->sub_ch[subch].center_freq;
        hparam->d_method = sub_channel_array->sub_ch[subch].raw_d_method;
        hparam->type = (type ==  STREAM_NIQ_TYPE_RAW ? DEFH_DTYPE_CH_IQ : DEFH_DTYPE_AUDIO);
    }
    hparam->ex_type = DFH_EX_TYPE_DEMO;
    ptr_header = xw_assamble_frame_data(&header_len, arg);
    if(ptr_header == NULL)
        return NULL;
#endif
    *hlen = header_len;

    return ptr_header;

}

int xspm_send_niq_data(void *data, size_t len, void *arg)
{
    #define _NIQ_SEND_TIMEOUT_US 10000
    size_t header_len = 0;
    struct _spm_xstream *pstream = spm_dev_get_stream(NULL);
    size_t _send_byte = 32768;
    uint8_t *hptr = NULL;
    int i, index;
    static struct timeval start, now;
    static bool time_start = true;
    
    if(data == NULL || len == 0 || arg == NULL)
        return -1;
    if(time_start){
        time_start = false;
        _spm_gettime(&start);
    }
    if(len < _send_byte){
        _spm_gettime(&now);
        if(_spm_tv_diff(&now, &start) > _NIQ_SEND_TIMEOUT_US && len >= 4096){
            index = len / 4096;
            len = index * 4096;
            _send_byte = len;
        }else{
            return -1;
        }
    }
    _spm_gettime(&start);
    if((hptr = _assamble_iq_header(0, 0, &header_len, _send_byte, arg, STREAM_NIQ_TYPE_RAW)) == NULL){
        printf_err("assamble head error\n");
        return -1;
    }
    
    uint8_t *pdata;
    struct iovec iov[2];
    iov[0].iov_base = hptr;
    iov[0].iov_len = header_len;
    index = len / _send_byte;
    pdata = (uint8_t *)data;
    __lock_iq_send__();
    for(i = 0; i<index; i++){
        iov[1].iov_base = pdata;
        iov[1].iov_len = _send_byte;
#if (defined CONFIG_PROTOCOL_DATA_TCP)
    tcp_send_vec_data(iov, 2, NET_DATA_TYPE_NIQ);
#else
    udp_send_vec_data(iov, 2, NET_DATA_TYPE_NIQ);
#endif
        pdata += _send_byte;
    }
    __unlock_iq_send__();
    
    xspm_read_xdma_data_over(-1, NULL, STREAM_NIQ);
    safe_free(hptr);
    
    return (int)(header_len + len);
}


int xspm_send_biq_data(int ch, void *data, size_t len, void *arg)
{
    size_t header_len = 0;
    struct _spm_xstream *pstream = spm_dev_get_stream(NULL);
    size_t _send_byte = (iq_send_unit_byte > 0 ? iq_send_unit_byte : DEFAULT_IQ_SEND_BYTE);
    uint8_t *hptr = NULL;
    
    if(data == NULL || len == 0 || arg == NULL)
        return -1;

    if(len < _send_byte)
        return -1;
    if((hptr = _assamble_iq_header(ch, 0, &header_len, _send_byte, arg, STREAM_BIQ_TYPE_RAW)) == NULL){
        printf_err("assamble head error\n");
        return -1;
    }
   
    int i, index;
    uint8_t *pdata;
    struct iovec iov[2];
    iov[0].iov_base = hptr;
    iov[0].iov_len = header_len;
    index = len / _send_byte;
    pdata = (uint8_t *)data;

    for(i = 0; i<index; i++){
        iov[1].iov_base = pdata;
        iov[1].iov_len = _send_byte;
#if (defined CONFIG_PROTOCOL_DATA_TCP)
    tcp_send_vec_data(iov, 2, NET_DATA_TYPE_BIQ);
#else
    udp_send_vec_data(iov, 2, NET_DATA_TYPE_BIQ);
#endif
        pdata += _send_byte;
    }
    xspm_read_xdma_data_over(-1, NULL, STREAM_BIQ);
    safe_free(hptr);
    
    return (int)(header_len + len);
}


static int xspm_read_stream_start(int ch, int subch, uint32_t len,uint8_t continuous, enum stream_type type)
{
    int index, reg, rc;
    struct _spm_xstream *pstream = spm_dev_get_stream(NULL);
    struct xdma_ring_trans_ioctl ring_trans;
    
    index = xspm_find_index_by_type(ch, -1, type);
    if(index < 0)
        return -1;

    memset(&ring_trans, 0, sizeof(struct xdma_ring_trans_ioctl));
    ring_trans.block_size = pstream[index].block_size;
    ring_trans.block_count = pstream[index].base.len / pstream[index].block_size;

    if(pstream[index].base.id < 0)
        return -1;
    printf_note("pstream[%d].id:%d, %s,%s\n", index, pstream[index].base.id, pstream[index].base.name, pstream[index].base.devname);
    rc = ioctl(pstream[index].base.id, IOCTL_XDMA_TRANS_START, &ring_trans);
    if (rc == 0) {
        printf_info("IOCTL_XDMA_TRANS_START succesful.\n");
    } 
    else {
        printf("ioctl(IOCTL_XDMA_TRANS_START) failed= %d\n", rc);
    }
    rc = _WaitDeviceReady(pstream[index].base.id);
    if (rc < 0) {
        printf("WaitDeviceReady failed %d\n", rc);
    }
    else if(rc == 0){
        printf("WaitDeviceReady timeout\n");
    }
    return 0;
}

static int xspm_read_stream_stop(int ch, int subch, enum stream_type type)
{
    struct _spm_xstream *pstream = spm_dev_get_stream(NULL);
    int index, ret, reg;
    struct xdma_ring_trans_ioctl ring_trans;

    index = xspm_find_index_by_type(ch, -1, type);
    if(index < 0)
        return -1;
    printf_info("name=%s, type=%d, id=%d\n", pstream[index].base.name, type, pstream[index].base.id);

    #if 1
    if(pstream[index].base.id < 0)
        return -1;
    ret = ioctl(pstream[index].base.id, IOCTL_XDMA_TRANS_STOP, &ring_trans);
    if (ret == 0) {
    	printf_info("IOCTL_XDMA_PERF_STOP succesful.\n");
    } 
    else {
    	printf_info("ioctl(IOCTL_XDMA_PERF_STOP) error:%d\n", ret);
    }
    #endif

    printf_debug("stream_stop: %d, %s\n", index, pstream[index].base.name);
    return 0;
}

static int _xspm_close(void *_ctx)
{
    struct spm_context *ctx = _ctx;
    int dev_len = 0;
    struct _spm_xstream *pstream = spm_dev_get_stream(&dev_len);
    int i, ch;

    for(i = 0; i< dev_len ; i++){
            if(pstream[i].base.rd_wr == DMA_READ)
                xspm_read_stream_stop(pstream[i].base.ch, -1, pstream[i].base.type);
        close(pstream[i].base.id);
    }
    for(ch = 0; ch< MAX_RADIO_CHANNEL_NUM; ch++){
        //safe_free(ctx->run_args[ch]->fft_ptr);
       // safe_free(ctx->run_args[ch]);
    }
    return 0;
}

static int xspm_xdma_data_clear(int ch,  void *arg, int type)
{
    struct spm_run_parm *run = arg;
    struct _spm_xstream *pstream = spm_dev_get_stream(NULL);
    int index = xspm_find_index_by_type(ch, -1, type);
    if(index < 0)
        return -1;
    
    xspm_read_stream_stop(ch, 0, pstream[index].base.type);
    xspm_read_stream_start(ch, 0, 0, 0, pstream[index].base.type);
    return 0;
}


static int xspm_read_xdma_data_over(int ch,  void *arg,  int type)
{
    int ret;
    struct _spm_xstream *pstream = spm_dev_get_stream(NULL);
    int index = xspm_find_index_by_type(ch, -1, type);
    if(index < 0)
        return -1;

    struct xdma_ring_trans_ioctl *ring_trans = &xinfo[index];

    printf_debug("*OVER index=%d block_size=%u,block_count=%u,ready_count=%u,rx_index=%u\n", index,
        ring_trans->block_size, ring_trans->block_count, ring_trans->ready_count, ring_trans->rx_index);
    ring_trans->invalid_index = ring_trans->rx_index;
    ring_trans->invalid_count = ring_trans->ready_count;
    if(pstream[index].base.id < 0)
        return -1;

    if(pstream){
        ret = ioctl(pstream[index].base.id, IOCTL_XDMA_TRANS_SET, ring_trans);
        if (ret){
            printf("ioctl(IOCTL_XDMA_TRANS_SET) failed:%d\n", ret);
            return -1;
        }
    }
    return 0;
}

static int xspm_read_xdma_fft_data_over(int ch,  void *arg)
{
    return xspm_read_xdma_data_over(ch, arg, STREAM_FFT);
}

static  float get_side_band_rate(uint32_t bandwidth)
{
    #define DEFAULT_SIDE_BAND_RATE  (1.28)
    float side_rate = 0.0;
     /* 根据带宽获取边带率 */
    if(config_read_by_cmd(EX_CTRL_CMD, EX_CTRL_SIDEBAND,0, &side_rate, bandwidth) == -1){
        printf_info("!!!!!!!!!!!!!SideRate Is Not Set In Config File[bandwidth=%u]!!!!!!!!!!!!!\n", bandwidth);
        return DEFAULT_SIDE_BAND_RATE;
    }
    return side_rate;
}

static int _spm_extract_half_point(void *data, int len, void *outbuf)
{
    fft_t *pdata = (fft_t *)data;
    for(int i = 0; i < len; i++){
        *(uint8_t *)outbuf++ = *pdata & 0x0ff;
        pdata++;
    }
    return len/2;
}

static int xspm_scan(uint64_t *s_freq_offset, uint64_t *e_freq, uint32_t *scan_bw, uint32_t *bw, uint64_t *m_freq)
{
    //#define MAX_SCAN_FREQ_HZ (6000000000)
    uint64_t _m_freq;
    uint64_t _s_freq, _e_freq;
    uint32_t _scan_bw, _bw;
    
    _s_freq = *s_freq_offset;
    _e_freq = *e_freq;
    _scan_bw = *scan_bw;

    {
        //_scan_bw = 175000000;
        if((_e_freq - _s_freq)/_scan_bw > 0){
            _bw = _scan_bw;
            *s_freq_offset = _s_freq + _scan_bw;
        }else{
            _bw = _e_freq - _s_freq;
            *s_freq_offset = _e_freq;
        }
        *scan_bw = _scan_bw;
        _m_freq = _s_freq + _bw/2;
        //fix bug:中频超6G无信号 wzq
        if (_m_freq > MAX_SCAN_FREQ_HZ){
            _m_freq = MAX_SCAN_FREQ_HZ;
        }
        *bw = _bw;
    }
    *m_freq = _m_freq;

    return 0;
}

/* 频谱数据整理 */
static fft_t *xspm_data_order(fft_t *fft_data, 
                                size_t fft_len,  
                                size_t *order_fft_len,
                                void *arg)
{
    struct spm_run_parm *run_args;
    float sigle_side_rate, side_rate;
    uint32_t single_sideband_size;
    uint64_t start_freq_hz = 0;
    int i;
    size_t order_len = 0, offset = 0;
    fft_t *p_buffer = NULL, *first = NULL;
    if(fft_data == NULL || fft_len == 0){
        printf_note("null data\n");
        return NULL;
    }
    
    run_args = (struct spm_run_parm *)arg;
    /* 获取边带率 */
    side_rate  =  get_side_band_rate(run_args->scan_bw);
    /* 去边带后FFT长度 */
    order_len = (size_t)((float)(fft_len) / side_rate + 0.5);
    /*双字节对齐*/
    order_len = order_len&0x0fffffffe; 
    

   //printf_note("side_rate = %f[fft_len:%lu, order_len=%lu], scan_bw=%u\n", side_rate, fft_len, order_len, run_args->scan_bw);
    // printf_warn("run_args->fft_ptr=%p, fft_data=%p, order_len=%u, fft_len=%u, side_rate=%f\n", run_args->fft_ptr, fft_data, order_len,fft_len, side_rate);
    /* 信号倒谱 */
    /*
       原始信号（注意去除中间边带）：==>真实输出信号；
       __                     __                             ___
         \                   /              ==》             /   \
          \_______  ________/                     _________/     \_________
                  \/                                                      
                 |边带  |
    */
    #if 1
    memcpy((uint8_t *)run_args->fft_ptr_swap,         (uint8_t *)(fft_data) , fft_len*2);
    memcpy((uint8_t *)run_args->fft_ptr,              (uint8_t *)(run_args->fft_ptr_swap + fft_len*2 -order_len), order_len);
    memcpy((uint8_t *)(run_args->fft_ptr+order_len),  (uint8_t *)run_args->fft_ptr_swap , order_len);
    #else
    memcpy((uint8_t *)run_args->fft_ptr,                (uint8_t *)(fft_data+fft_len -order_len/2) , order_len);
    memcpy((uint8_t *)(run_args->fft_ptr+order_len),    (uint8_t *)fft_data , order_len);
    #endif

    p_buffer =  (fft_t *)run_args->fft_ptr;

#if defined(CONFIG_SPM_FFT_EXTRACT_POINT)
    order_len =  _spm_extract_half_point(p_buffer, order_len, run_args->fft_ptr_swap);
    p_buffer = run_args->fft_ptr_swap;
#endif
    if(run_args->mode == OAL_FAST_SCAN_MODE || run_args->mode ==OAL_MULTI_ZONE_SCAN_MODE){
        if(run_args->scan_bw > run_args->bandwidth){
             uint64_t start_freq_hz = 0;
             /* 扫描最后一段，射频中心频率为实际剩余带宽的中心频率，需要去掉多余部分 */
             start_freq_hz = run_args->s_freq_offset - (run_args->m_freq_s - run_args->scan_bw/2);
             offset = ((float)order_len * ((float)start_freq_hz/(float)run_args->scan_bw));
             order_len = ((float)order_len * ((float)run_args->bandwidth/(float)run_args->scan_bw));
        }
    }
    first = p_buffer + offset;
#if defined(CONFIG_SPM_BOTTOM)
        if(bottom_noise_cali_en())
            bottom_calibration(run_args->ch, first, fft_len, run_args->fft_size, run_args->m_freq, run_args->bandwidth);
        bottom_deal(run_args->ch, first, fft_len, run_args->fft_size, run_args->m_freq, run_args->bandwidth);
#endif
    *order_fft_len = order_len;
    printf_debug("order_len=%lu, offset = %lu, start_freq_hz=%"PRIu64", s_freq_offset=%"PRIu64", m_freq=%"PRIu64", scan_bw=%u,fft_len=%lu\n", 
        order_len, offset, start_freq_hz, run_args->s_freq_offset, run_args->m_freq_s, run_args->scan_bw,fft_len);

    return (fft_t *)first;
}


static const struct spm_backend_ops xspm_ops = {
    .create = xspm_create,
    .read_fft_data = xspm_read_fft_data,
    .read_fft_vec_data = xspm_read_fft_vec_data,
    .send_fft_data = xspm_send_fft_data,
    .read_iq_vec_data = xspm_read_iq_vec_data,
    .read_raw_vec_data = xspm_read_xdma_raw_data,
    .read_raw_over_deal = xspm_read_xdma_raw_data_over,
    .send_data_by_fd = xspm_send_data_by_fd,
    .write_data = xspm_stram_write,
    //.read_niq_data = xspm_read_niq_data,
    //.read_biq_data = xspm_read_biq_data,
    .read_fft_over_deal = xspm_read_xdma_fft_data_over,
    .send_biq_data = xspm_send_biq_data,
    .send_niq_data = xspm_send_niq_data,
    .stream_start = xspm_read_stream_start,
    .stream_stop = xspm_read_stream_stop,
    .data_order = xspm_data_order,
    .spm_scan = xspm_scan,
    .close = _xspm_close,
};

struct spm_context * spm_create_context(void)
{
    int ret = -ENOMEM, ch;
    unsigned int len;
    struct spm_context *ctx = zalloc(sizeof(*ctx));
    if (!ctx)
        goto err_set_errno;

    ctx->ops = &xspm_ops;
    ctx->pdata = &config_get_config()->oal_config;
#ifdef CONFIG_FILE_SINK
    file_sink_init(get_sink_file_path_name(), FILE_SINK_TYPE_FFT, get_sink_file_time_ms());
    file_sink_init(NULL, FILE_SINK_TYPE_NIQ, get_sink_file_time_ms());
    file_sink_init(NULL, FILE_SINK_TYPE_BIQ, get_sink_file_time_ms());
#endif
    printf_info("create xdma ctx\n");
err_set_errno:
    errno = -ret;
    return ctx;

}


