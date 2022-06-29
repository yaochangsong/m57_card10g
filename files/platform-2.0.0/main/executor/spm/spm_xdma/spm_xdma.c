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



static int xspm_read_stream_stop(int ch, int subch, enum stream_type type);
static int xspm_read_xdma_data_over(int ch,  void *arg,  int type);
static int xspm_xdma_data_clear(int ch,  void *arg, int type);


static inline void *zalloc(size_t size)
{
	return calloc(1, size);
}

FILE *_file_fd[64] = {0};
static  void init_write_file(char *filename, int index)
{
    _file_fd[index] = fopen(filename, "w+b");
    if(!_file_fd[index]){
        printf_err("Open file error!\n");
    }
}

static inline void create_file_path(int index)
{
    char buffer[64];
    
    if(_file_fd[index] > 0)
        return;
    
    sprintf(buffer, "/tmp/ch_%d", index);
    init_write_file(buffer, index);
    printf_info("create file: %s\n", buffer);
}

static inline void create_tmp_file(int index, int chip_id, int func_id, int prio_id, int port)
{
    char buffer[64];
    
    if(_file_fd[index] > 0)
        return;
    
    sprintf(buffer, "/tmp/c%x_f%x_prio%x_port%x", chip_id,func_id,prio_id,port);
    init_write_file(buffer, index);
    printf_info("create file: %s\n", buffer);
}


static inline void close_file(void)
{
    for(int i = 0; i < 64; i++){
        if(_file_fd[i] > 0){
            fclose(_file_fd[i]);
        }
    }
    
}


static inline int write_file(uint8_t *pdata, int index, int len)
{
    char _file_buffer[32] = {0};
    uint8_t *ptr = pdata;
    for(int i = 0; i < len; i++){
        sprintf(_file_buffer, "0x%02x ", *ptr++);
        fwrite((void *)_file_buffer,1, strlen(_file_buffer), _file_fd[index]);
    }
    sync();

    return 0;
}

static inline int write_file_nbyte(int8_t *pdata, int n, int index)
{
    fwrite((void *)pdata, sizeof(int8_t), n, _file_fd[index]);
    return 0;
}


static inline int write_file_nfft(int16_t *pdata, int n, int index)
{
    fwrite((void *)pdata, sizeof(int16_t), n, _file_fd[index]);
    return 0;
}
static inline int write_lf(int index)
{
    char lf = '\n';
    fwrite((void *)&lf,1, 1, _file_fd[index]);
    return 0;
}

static inline int write_over(int index, uint32_t len)
{
    char buffer[128];
    sprintf(buffer, "\n------------read frame over: %u[0x%x]---------------\n", len,len);
    fwrite((void *)buffer, 1, strlen(buffer), _file_fd[index]);
    sync();
    return 0;
}

static void print_array(uint8_t *ptr, ssize_t len)
{
    if(ptr == NULL || len <= 0)
        return;
    
    for(int i = 0; i< len; i++){
        printf("%02x ", *ptr++);
    }
    printf("\n");
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

    pstream = spm_dev_get_stream(&dev_len);
    /* create stream */
    for(i = 0; i< dev_len ; i++){
        pstream[i].id = open(pstream[i].devname, O_RDWR);
        if( pstream[i].id < 0){
            fprintf(stderr, "[%d]open:%s, %s\n", i, pstream[i].devname, strerror(errno));
            continue;
        }
        printf_info("mmap: %s\n", pstream[i].devname);
        memset(&ring_trans, 0, sizeof(struct xdma_ring_trans_ioctl));
        ring_trans.block_size = pstream[i].block_size;
        ring_trans.block_count = pstream[i].len / pstream[i].block_size;
        printf_info("block_size=%u, block_count=%u, len=%u\n", ring_trans.block_size, ring_trans.block_count, pstream[i].len);
        xspm_read_stream_stop(pstream[i].ch, -1, pstream[i].type);
        if(pstream[i].rd_wr == DMA_READ){
        rc = ioctl(pstream[i].id, IOCTL_XDMA_INIT_BUFF, &ring_trans);  //close时释放
        if (rc == 0) {
            printf_info("IOCTL_XDMA_INIT_BUFF succesful.\n");
        } 
        else {
            printf("ioctl(IOCTL_XDMA_INIT_BUFF) failed= %d\n", rc);
            exit(-1);
        }
            printf_info("[%d, ch=%d]create stream[%s] dev:%s len=%u, block_size:%u, block_count=%u\n", pstream[i].id,pstream[i].ch, pstream[i].name, 
                                pstream[i].devname, pstream[i].len, pstream[i].block_size, ring_trans.block_count);
            for(int j = 0; j < ring_trans.block_count; j++){
                pstream[i].ptr[j] = mmap(NULL, ring_trans.block_size, PROT_READ | PROT_WRITE,MAP_SHARED, pstream[i].id, j * pagesize);
                if (pstream[i].ptr[j] == (void*) -1) {
                    fprintf(stderr, "mmap: %s\n", strerror(errno));
                    exit(-1);
                }
                printf_info("block[%d]: ptr=%p, pagesize=%d\n", j, pstream[i].ptr[j], pagesize);
            }
        }
        usleep(1000);
    }
    
    return 0;
}


struct xdma_ring_trans_ioctl xinfo[4];
static ssize_t xspm_stream_read(int ch, int type,  void **data, uint32_t *len, void *args)
{
    #define _STREAM_READ_TIMEOUT_MS (1000)
    int rc = 0;
    struct _spm_xstream *pstream = spm_dev_get_stream(NULL);
    struct xdma_ring_trans_ioctl *info = &xinfo[type];
    size_t readn = 0;
    static uint32_t timer = 0;
    struct timeval start, now;
    
    memset(info, 0, sizeof(struct xdma_ring_trans_ioctl));
    if(pstream[type].id < 0){
        printf_debug("%d stream node:%s not found\n",type, pstream[type].name);
        return -1;
    }
     _spm_gettime(&start);
    do{
        rc =  ioctl(pstream[type].id, IOCTL_XDMA_TRANS_GET, info);
        if (rc) {
            printf_err("type=%d, id=%d ioctl(IOCTL_XDMA_TRANS_GET) failed %d, info=%p, %p, %p\n",type, pstream[type].id, rc, info, &xinfo[0], &xinfo[1]);
            return -1;
        }
        if(info->status == RING_TRANS_OK){
            printf_debug("type=%d\n", type);
            printf_debug("ring_trans.rx_index:%d\n", info->rx_index);
            printf_debug("ring_trans.ready_count:%d\n", info->ready_count);
            printf_debug("ring_trans.status:%d\n", info->status);
        } else if(info->status == RING_TRANS_FAILED){
            printf_err("status:RING_TRANS_FAILED.\n");
            return -1;
        } else if(info->status == RING_TRANS_OVERRUN){
            printf_warn("*****status:RING_TRANS_OVERRUN.*****\n");
            xspm_xdma_data_clear(ch, args, type);
        } else if(info->status == RING_TRANS_INITIALIZING){
            printf_warn("*****status:RING_TRANS_INITIALIZING.*****\n");
            usleep(10);
        } else if(info->status == RING_TRANS_PENDING){
            //printf_warn("*****status:RING_TRANS_PENDING\n");
            usleep(1);
        }
        _spm_gettime(&now);
        if(_spm_tv_diff(&now, &start) > _STREAM_READ_TIMEOUT_MS){
            printfi("Read TimeOut![%u]\r",timer++);
            break;
        }
    }while(info->status == RING_TRANS_PENDING);

   int index;
    uint8_t *ptr = NULL;
    //printf_note("ready_count: %u, type:%d\n", info->ready_count, type);
    for(int i = 0; i < info->ready_count; i++){
        index = (info->rx_index + i) % info->block_count;
        data[i] = pstream[type].ptr[index];
        len[i] = info->results[index].length;
        timer = 0;
        printf_debug("[%d,index:%d][%p, %p, len:%u, offset=0x%x]%s\n", 
                i, index, data[i], pstream[type].ptr[index], len[i], info->rx_index,  pstream[type].name);
    }
    return info->ready_count;
}



static inline int xspm_find_index_by_type(int ch, int subch, enum stream_type type)
{
    int dev_len = 0;
    struct _spm_xstream *pstream = spm_dev_get_stream(&dev_len);
    int i, find = 0, index = -1;

    subch = subch;
    for(i = 0; i < dev_len; i++){
        if((type == pstream[i].type) && (ch == pstream[i].ch || pstream[i].ch == -1)){
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
        if(rw == pstream[i].rd_wr && ch == pstream[i].ch){
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

    index = xspm_find_index_by_rw(ch, -1, DMA_WRITE);
    if(index < 0)
        return -1;
    
    if(pstream[index].id < 0)
        return -1;

    while (buflen) {
        len =  write(pstream[index].id, buf, buflen);
        if (len < 0) {
            printf_note("[fd:%d]-send len : %ld, %d[%s][%s], %d, %p\n", pstream[index].id, len, errno, strerror(errno), pstream[index].name, buflen, buf);
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
    return ret;
}

static ssize_t xspm_read_fft_data(int ch, void **data, void *args)
{
    struct spm_run_parm *run_args = args;
    ssize_t count = 0, fft_byte_len = 0;
    int index;
    uint32_t  len[XDMA_TRANSFER_MAX_DESC] = {0};
    
    index = xspm_find_index_by_type(ch, -1, STREAM_FFT);
    if(index < 0)
        return -1;
    count = xspm_stream_read(ch, index, data, len, args);
    count = count;
    fft_byte_len = len[0];
    
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
    if(hparam->ch == 0)
        __lock_fft_send__();
    else
        __lock_fft2_send__();
#if (defined CONFIG_PROTOCOL_DATA_TCP)
    tcp_send_vec_data(iov, 2, NET_DATA_TYPE_FFT);
#else
    udp_send_vec_data(iov, 2, NET_DATA_TYPE_FFT);
#endif
    if(hparam->ch == 0)
        __unlock_fft_send__();
    else
        __unlock_fft2_send__();
    safe_free(ptr_header);
    return (header_len + data_byte_size);
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
    ring_trans.block_count = pstream[index].len / pstream[index].block_size;

    if(pstream[index].id < 0)
        return -1;
    
    rc = ioctl(pstream[index].id, IOCTL_XDMA_TRANS_START, &ring_trans);
    if (rc == 0) {
        printf_info("IOCTL_XDMA_TRANS_START succesful.\n");
    } 
    else {
        printf("ioctl(IOCTL_XDMA_TRANS_START) failed= %d\n", rc);
    }
    rc = _WaitDeviceReady(pstream[index].id);
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
    printf_info("name=%s, type=%d, id=%d\n", pstream[index].name, type, pstream[index].id);

    #if 1
    if(pstream[index].id < 0)
        return -1;
    ret = ioctl(pstream[index].id, IOCTL_XDMA_TRANS_STOP, &ring_trans);
    if (ret == 0) {
    	printf_info("IOCTL_XDMA_PERF_STOP succesful.\n");
    } 
    else {
    	printf_info("ioctl(IOCTL_XDMA_PERF_STOP) error:%d\n", ret);
    }
    #endif

    printf_debug("stream_stop: %d, %s\n", index, pstream[index].name);
    return 0;
}

static int _xspm_close(void *_ctx)
{
    struct spm_context *ctx = _ctx;
    int dev_len = 0;
    struct _spm_xstream *pstream = spm_dev_get_stream(&dev_len);
    int i, ch;

    for(i = 0; i< dev_len ; i++){
            if(pstream[i].rd_wr == DMA_READ)
                xspm_read_stream_stop(pstream[i].ch, -1, pstream[i].type);
        close(pstream[i].id);
    }
    for(ch = 0; ch< MAX_RADIO_CHANNEL_NUM; ch++){
        //safe_free(ctx->run_args[ch]->fft_ptr);
       // safe_free(ctx->run_args[ch]);
    }
    close_file();
    return 0;
}

static int xspm_xdma_data_clear(int ch,  void *arg, int type)
{
    struct spm_run_parm *run = arg;
    struct _spm_xstream *pstream = spm_dev_get_stream(NULL);
    int index = xspm_find_index_by_type(ch, -1, type);
    if(index < 0)
        return -1;
    
    xspm_read_stream_stop(ch, 0, pstream[index].type);
    xspm_read_stream_start(ch, 0, 0, 0, pstream[index].type);
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
    if(pstream[index].id < 0)
        return -1;

    if(pstream){
        ret = ioctl(pstream[index].id, IOCTL_XDMA_TRANS_SET, ring_trans);
        if (ret){
            //printf("ioctl(IOCTL_XDMA_TRANS_SET) failed:%d\n", ret);
            return -1;
        }
    }
    return 0;
}


static const struct spm_backend_ops xspm_ops = {
    .create = xspm_create,
    .read_fft_data = xspm_read_fft_data,
    .send_fft_data = xspm_send_fft_data,
    .stream_start = xspm_read_stream_start,
    .stream_stop = xspm_read_stream_stop,
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
    printf_info("create xdma ctx\n");
err_set_errno:
    errno = -ret;
    return ctx;

}


