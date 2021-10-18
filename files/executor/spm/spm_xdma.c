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
#include "spm.h"
#include "spm_xdma.h"
#include "../../bsp/io.h"
#include "../../lib/libubox/list.h"
#include "../../net/net_sub.h"

#define  SPM_DISPATCHER_ON
#define  SPM_HEADER_CHECK

static int xspm_read_stream_stop(int ch, int subch, enum stream_type type);
static int xspm_read_xdma_data_over(int ch,  void *arg);
static int xspm_xdma_data_clear(int ch,  void *arg);


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
    printf_note("create file: %s\n", buffer);
}

static inline void create_tmp_file(int index, int chip_id, int func_id, int prio_id, int port)
{
    char buffer[64];
    
    if(_file_fd[index] > 0)
        return;
    
    sprintf(buffer, "/tmp/c%x_f%x_prio%x_port%x", chip_id,func_id,prio_id,port);
    init_write_file(buffer, index);
    printf_note("create file: %s\n", buffer);
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

#define RUN_MAX_TIMER 1
static struct timeval *runtimer[RUN_MAX_TIMER];
static int32_t _init_run_timer(uint8_t ch)
{
    int i;
    for(i = 0; i< RUN_MAX_TIMER ; i++){
        runtimer[i] = (struct timeval *)malloc(sizeof(struct timeval)*ch);
        memset(runtimer[i] , 0, sizeof(struct timeval)*ch);
    }
    
    return 0;
}

static int32_t _get_run_timer(uint8_t index, uint8_t ch)
{
    struct timeval *oldTime; 
    struct timeval newTime; 
    int32_t _t_us, ntime_us; 
    if(ch >= MAX_RADIO_CHANNEL_NUM){
        return -1;
    }
    if(index >= RUN_MAX_TIMER){
        return -1;
    }
    oldTime = runtimer[index]+ch;
     if(oldTime->tv_sec == 0 && oldTime->tv_usec == 0){
        gettimeofday(oldTime, NULL);
        return 0;
    }
    gettimeofday(&newTime, NULL);
    //printf("newTime:%zd,%zd\n",newTime.tv_sec, newTime.tv_usec);
    //printf("oldTime:%zd,%zd\n",oldTime->tv_sec, oldTime->tv_usec);
    _t_us = (newTime.tv_sec - oldTime->tv_sec)*1000000 + (newTime.tv_usec - oldTime->tv_usec); 
    //printf("_t_us:%d\n",_t_us);
    if(_t_us > 0)
        ntime_us = _t_us; 
    else 
        ntime_us = 0; 
    
    return ntime_us;
}

static int32_t _reset_run_timer(uint8_t index, uint8_t ch)
{
    struct timeval newTime; 
    struct timeval *oldTime; 
    if(ch >= MAX_RADIO_CHANNEL_NUM){
        return -1;
    }
    if(index >= RUN_MAX_TIMER){
        return -1;
    }
    gettimeofday(&newTime, NULL);
    oldTime = runtimer[index]+ch;
    memcpy(oldTime, &newTime, sizeof(struct timeval)); 
    return 0;
}


static struct _spm_xstream spm_xstream[] = {
        {XDMA_R_DEV0,        -1, 0, XDMA_BUFFER_SIZE, XDMA_BLOCK_SIZE, "Write XDMA Stream", XDMA_WRITE, XDMA_STREAM, -1},
        {XDMA_R_DEV1,        -1, 0, XDMA_BUFFER_SIZE, XDMA_BLOCK_SIZE, "Read XDMA Stream0",  XDMA_READ,  XDMA_STREAM, -1},
        {XDMA_R_DEV2,        -1, 1, XDMA_BUFFER_SIZE, XDMA_BLOCK_SIZE, "Read XDMA Stream1",  XDMA_READ,  XDMA_STREAM, -1},
};

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
			printf("ioctl(IOCTL_XDMA_STATUS) failed %d\n", ret);
			return -1;	  
		}	

		printf("IOCTL_XDMA_STATUS:%d\n", status);
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
    pstream = spm_xstream;
    int i = 0, rc;
    int pagesize = getpagesize();
    printf_note("SPM init.%ld\n", ARRAY_SIZE(spm_xstream));
    
    /* create stream */
    for(i = 0; i< ARRAY_SIZE(spm_xstream) ; i++){
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
        xspm_read_stream_stop(pstream[i].ch, -1, XDMA_STREAM);
        if(pstream[i].rd_wr == XDMA_READ){
        rc = ioctl(pstream[i].id, IOCTL_XDMA_INIT_BUFF, &ring_trans);  //close时释放
        if (rc == 0) {
            printf("IOCTL_XDMA_INIT_BUFF succesful.\n");
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
    #define _STREAM_READ_TIMEOUT_US (1000000)
    int rc = 0;
    struct _spm_xstream *pstream;
    pstream = spm_xstream;
    struct xdma_ring_trans_ioctl *info = &xinfo[type];
    size_t readn = 0;
    static uint32_t timer = 0;
    
    memset(info, 0, sizeof(struct xdma_ring_trans_ioctl));
    
    if(pstream[type].id < 0){
        printf_debug("%d stream node:%s not found\n",type, pstream[type].name);
        return -1;
    }
    _reset_run_timer(0, 0);
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
            xspm_xdma_data_clear(0, args);
        } else if(info->status == RING_TRANS_INITIALIZING){
            printf_warn("*****status:RING_TRANS_INITIALIZING.*****\n");
            usleep(10);
        } else if(info->status == RING_TRANS_PENDING){
            //printf_warn("*****status:RING_TRANS_PENDING\n");
            usleep(1);
        }
           
        if(_get_run_timer(0, 0) > _STREAM_READ_TIMEOUT_US){
            printfn("Read TimeOut![%u]\r",timer++);
            break;
            //return -1;
        }
    }while(info->status == RING_TRANS_PENDING);

   int index;
    uint8_t *ptr = NULL;
    printf_debug("ready_count: %u\n", info->ready_count);
    for(int i = 0; i < info->ready_count; i++){
        index = (info->rx_index + i) % info->block_count;
        data[i] = pstream[type].ptr[index];
        len[i] = info->results[index].length;
        printf_debug("data=%p, len=%u\n", data[i], len[i]);
        timer = 0;
		#if 0
        if(i < 64){
            printf_note("[%d]len: %u\n", i, len[i]);
            ptr = data[i];
            print_array(ptr+len[i]-32, 32);
            //create_file_path(i);
            //write_file(data[i], i, len[i]);
            //write_over(i, len[i]);
        }
		#endif
        printf_info("[%d,index:%d][%p, %p, len:%u, offset=0x%x]%s\n", 
                i, index, data[i], pstream[type].ptr[index], len[i], info->rx_index,  pstream[type].name);
    }
    return info->ready_count;
}



static inline int xspm_find_index_by_type(int ch, int subch, enum stream_type type)
{
    struct _spm_xstream *pstream = spm_xstream;
    int i, find = 0, index = -1;

    subch = subch;
    for(i = 0; i < ARRAY_SIZE(spm_xstream); i++){
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
    struct _spm_xstream *pstream = spm_xstream;
    int i, find = 0, index = -1;

    subch = subch;
    for(i = 0; i < ARRAY_SIZE(spm_xstream); i++){
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
    struct _spm_xstream *pstream;

    if (!buflen)
        return 0;

    pstream = spm_xstream;
    
    //index = xspm_find_index_by_type(ch, -1, XDMA_STREAM);
    index = xspm_find_index_by_rw(ch, -1, XDMA_WRITE);
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

static ssize_t xspm_stream_read_test(int ch, int type,  void **data, uint32_t *len, void *args)
{
    static uint8_t buffer[8192] = {
        0x51,0x57,0x1f,0x30,0x18,0x01,0x00,0x00,0x00,0x00,0x02,0x05,0x00,0x00,0x08,0x01,
        0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0xaa,0xaa,0x55,0x55,0xfa,0x0b,0x00,0x00,
        0x30,0x05,0x78,0x09,0x36,0x05,0xdc,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x25,0x10,0x18,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x55,0x55,0xaa,0xaa,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
        0x51,0x57,0x1f,0x30,0x18,0x01,0x00,0x00,0x00,0x00,0x01,0x05,0x00,0x00,0x08,0x01,
        0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0xaa,0xaa,0x55,0x55,0xfa,0x0b,0x00,0x00,
        0x30,0x05,0x78,0x09,0x36,0x05,0xdc,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x25,0x10,0x18,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x55,0x55,0xaa,0xaa,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
    };

    int count = 10;
    for(int i= 0; i < count; i++){
         data[i] = buffer;
         len[i] = sizeof(buffer);
       // print_array(data[i], 32);
    }
    usleep(10);
    //sleep(1);
    return count;
}

static ssize_t xspm_stream_read_test2(int ch, int type,  void **data, uint32_t *len, void *args)
{
    static uint8_t buffer[8192] = {
        0x51,0x57,0xbe,0x30,0x18,0x01,0x00,0x00,0x00,0x00,
        0x01,0x05,0x00,0x00,0x08,0x01,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0xaa,0xaa,
        0x55,0x55,0xc8,0x02,0x00,0x00,0x30,0x05,0x74,0x09,0x38,0x05,0xc9,0x09,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x25,0x10,0x18,0x20,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x55,0x55,0xaa,0xaa
    };
    static uint8_t buffer2[8192] = {
        0x51,0x57,0x1f,0x30,0x18,0x01,0x00,0x00,0x00,0x00,0x02,0x05,0x00,0x00,0x08,0x01,
        0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0xaa,0xaa,0x55,0x55,0xfa,0x0b,0x00,0x00,
        0x30,0x05,0x78,0x09,0x36,0x05,0xdc,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x25,0x10,0x18,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x55,0x55,0xaa,0xaa,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
        0x51,0x57,0xbe,0x30,0x18,0x01,0x00,0x00,0x00,0x00,
        0x02,0x05,0x00,0x00,0x08,0x01,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0xaa,0xaa,
        0x55,0x55,0xc8,0x02,0x00,0x00,0x30,0x05,0x74,0x09,0x38,0x05,0xc9,0x09,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x25,0x10,0x18,0x20,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x55,0x55,0xaa,0xaa,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
        0x51,0x57,0xbe,0x30,0x18,0x01,0x00,0x00,0x00,0x00,
        0x02,0x06,0x00,0x00,0x08,0x01,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0xaa,0xaa,
        0x55,0x55,0xc8,0x02,0x00,0x00,0x30,0x05,0x74,0x09,0x38,0x05,0xc9,0x09,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x25,0x10,0x18,0x20,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x55,0x55,0xaa,0xaa,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 
    };
    #define _read_cnt 1
    static uint8_t **alloc_mem = NULL;
    if(alloc_mem == NULL){
        alloc_mem = calloc(_read_cnt, sizeof(*alloc_mem));
        if (!alloc_mem) {
            return -ENOMEM;
        }
    }

    for(int i = 0; i < _read_cnt; i++){
        if(alloc_mem[i] == NULL){
            alloc_mem[i] = calloc(1, 8192);
            if (!alloc_mem[i]) {
                return -ENOMEM;
            }
        }
       // if(i < _read_cnt - 3)
            memcpy(alloc_mem[i], buffer2, 8192);
       // else
       //     memcpy(alloc_mem[i], buffer2, 8192);
        data[i] = alloc_mem[i];
        len[i] = 8192;
    }
    //usleep(500000);
    sleep(1);
    return _read_cnt;
}




//static ssize_t xspm_read_xdma_data(int ch, void **data, void *args)
static ssize_t xspm_read_xdma_data(int ch , void **data, uint32_t *len, void *args)
{
    int index;
    struct net_sub_st *parg = args;

    index = xspm_find_index_by_rw(ch, -1, XDMA_READ);
    if(index < 0)
        return -1;
    //test
    #if 0
    if(parg){
        parg->chip_id = 0x502;
        parg->func_id = 0x6;
    }
    #endif

    //return xspm_stream_read_test(ch, index, data, len, args);
    return xspm_stream_read(ch, index, data, len, args);
}

static inline void xdma_data_dispatcher_refresh(int ch, void *args)
{
    struct spm_run_parm *arg = args;
    
    arg->xdma_disp.type_num = 0;
    for(int i = 0; i < MAX_XDMA_DISP_TYPE_NUM; i++){
        arg->xdma_disp.type[i]->vec_cnt = 0;
    }
}

static ssize_t _xdma_of_match_pkgs(void *data, uint32_t len, void *args, size_t offp)
{
    struct data_frame_st{
        uint16_t py_dist_addr;
        uint16_t py_src_addr;
        uint16_t dist_addr;
        uint16_t len;
        uint16_t type;
        uint16_t src_addr;
        uint16_t port;
    }__attribute__ ((packed));
    struct data_frame_st *pdata[4096];
    struct net_sub_st *psub = args;
    uint8_t *payload = NULL;
    uint8_t *ptr = data;
    uint16_t py_src_addr = 0, src_addr = 0, port = 0;
    int pos = 0, reminder16 = 0;
    size_t sum_len = 0, frame_len = 0, raw_len = 0;

    do{
        if(*(uint16_t *)ptr != 0x5751){  /* header */
            if(sum_len != 0){
                if(len < offp)
                    printf_warn("maybe payload len err, offset err: %lu, len:%u\n", offp, len);
                if(sum_len == (len - offp)){
                    printf_info("read over block size: %lu, offp=%lu\n", sum_len, offp);
                } else if(sum_len <  (len - offp)){
                    printf_warn("err header=0x%x, block size left:%ld, offp=%lu\n", *(uint16_t *)ptr, len - offp - sum_len, offp);
                } else{
                    printf_warn("err!! comsume length [%lu] err!, %u, %lu\n", sum_len, len, offp);
                }
            }
            break;
        }
        payload = ptr + 8;
        pdata[pos] = (struct data_frame_st *)payload;
        frame_len = *(uint16_t *)(ptr + 4); //aglin 16byte
        reminder16 = frame_len % 16;
        if(reminder16 != 0){
            raw_len = frame_len;
            frame_len += reminder16;
            printf_info("[%d]not aglin 16,add %d, frame_len:%lu[0x%lx], raw_len:%lu[0x%lx]\n", pos, reminder16, frame_len, frame_len,raw_len,raw_len);
        }
        if(frame_len > 2048){
            printf_warn("payload length error: %lu\n", frame_len);
            break;
        }
        
        if(pos == 0){
            psub->chip_id = py_src_addr = pdata[pos]->py_src_addr;
            psub->func_id = src_addr = pdata[pos]->src_addr;
            psub->prio_id = ((*(uint8_t *)(ptr + 3) >> 4) & 0x0f) == 0 ? 0 :1;
            psub->port = port = pdata[pos]->port;
           // print_array(ptr, len); 
            printf_note("0 frame_len：%lu, chip_id=0x%x, func_id=0x%x, prio_id=0x%x,port: 0x%x\n", frame_len, psub->chip_id, psub->func_id, psub->prio_id, psub->port);
        } else if(py_src_addr != pdata[pos]->py_src_addr || src_addr != pdata[pos]->src_addr || port != pdata[pos]->port){
            //printf_note("func_id not eq\n");
            break;
        } else{
            //printf_note("same type: %d, py_src_addr=%x, src_addr=%x\n", pos,  pdata[pos]->py_src_addr, pdata[pos]->src_addr);
        }
        pos++;
        if(pos >= 4096){
            printf_warn(">>>>>>>>>>pos is overrun!\n");
            break;
        }
            
        
        sum_len += frame_len;
        ptr = ptr +frame_len ;
    }while(sum_len < (XDMA_BLOCK_SIZE - offp));
    //printf_note("sum_len:%lu, pos:%d\n", sum_len, pos);
    return sum_len;
} 

static ssize_t _xdma_of_match_pkgs_test(void *data, uint32_t len, void *args, size_t offp)
{
    struct data_frame_st{
        uint16_t py_dist_addr;
        uint16_t py_src_addr;
        uint16_t dist_addr;
        uint16_t len;
        uint16_t type;
        uint16_t src_addr;
        uint16_t cmd;
    }__attribute__ ((packed));
    struct data_frame_st *pdata[2048];
    struct net_sub_st *psub = args;
    uint8_t *payload = NULL;
    uint8_t *ptr = data;
    uint16_t py_src_addr = 0, src_addr = 0;
    int pos = 0, reminder16 = 0;
    size_t sum_len = 0, frame_len = 0;
    static int rev = 0;

    do{
        if(*(uint16_t *)ptr != 0x5751){  /* header */
            //printf_warn("err header=0x%x, block size left:%ld\n", *(uint16_t *)ptr, XDMA_BLOCK_SIZE - offp);
           // break;
        }
        payload = ptr + 8;
        pdata[pos] = (struct data_frame_st *)payload;
        frame_len = 64*1024;//*(uint16_t *)(ptr + 4); //aglin 8byte
        //reminder16 = frame_len % 16;
        if(reminder16 != 0){
            frame_len += reminder16;
           // printf_note("[%d]not aglin 16,add %d, frame_len:%lu\n", pos, reminder16, frame_len);
        }
            
        if(pos == 0){
            psub->chip_id = py_src_addr = 0x0502;//pdata[pos]->py_src_addr;
            //if(rev == 0)
                psub->func_id = src_addr = 3;//pdata[pos]->src_addr;
            //else
            //    psub->func_id = src_addr = 6;
            rev = !rev;
            psub->prio_id = 1;//(*(uint8_t *)(ptr + 3) >> 4) & 0x0f == 0 ? 0 :1;
            psub->port = 1;
            printf_info("0 frame_len：%lu, chip_id=0x%x, func_id=0x%x, prio_id=0x%x\n", frame_len, psub->chip_id, psub->func_id, psub->prio_id);
        } //else
          //  break;
        pos++;
        sum_len += frame_len;
        ptr = ptr +frame_len ;
    }while(sum_len < (XDMA_BLOCK_SIZE - offp));
    printf_info("sum_len:%lu, pos=%d, offp=%lu\n", sum_len, pos, offp);
    return sum_len;
} 


static int _xdma_load_disp_buffer(void *data, ssize_t len, void *args, void *run)
{
    int hashid = 0, vec_cnt = 0;
    struct net_sub_st *sub = args;
    struct spm_run_parm *prun = run; 
    
    hashid = GET_HASHMAP_ID(sub->chip_id, sub->func_id, sub->prio_id, sub->port);
    printf_info("hashid:%d, chip_id:%x, func_id:%x, prio_id:%x,port:%x\n", hashid, sub->chip_id, sub->func_id, sub->prio_id, sub->port);
    
    if(hashid > MAX_XDMA_DISP_TYPE_NUM || hashid < 0 || prun->xdma_disp.type[hashid] == NULL){
        printf_err("hash id err[%d]\n", hashid);
        return -1;
    }
    vec_cnt = prun->xdma_disp.type[hashid]->vec_cnt;
    prun->xdma_disp.type[hashid]->subinfo.chip_id = sub->chip_id;
    prun->xdma_disp.type[hashid]->subinfo.func_id = sub->func_id;
    prun->xdma_disp.type[hashid]->vec[vec_cnt].iov_base = data;
    prun->xdma_disp.type[hashid]->vec[vec_cnt].iov_len = len;
    prun->xdma_disp.type_num++;
    prun->xdma_disp.type[hashid]->vec_cnt++;
    prun->xdma_disp.inout.in_bytes += len;
    //printf_note("vec_cnt:%d\n", prun->xdma_disp.type[hashid]->vec_cnt);
    #if 0
    create_tmp_file(hashid%64, sub->chip_id, sub->func_id, sub->prio_id, sub->port);
    printf_note("index:%d, %x, %x, %x, %x\n", hashid%64, sub->chip_id, sub->func_id, sub->prio_id, sub->port);
    write_file_nbyte(data, len, hashid%64);
    #endif
    //write_file(data[i], i, len[i]);
    //write_over(i, len[i]);
    return 0;
}

static inline bool _data_check(const void *data, uint32_t len, int index)
{
    static uint32_t cnt = 0, k = 0;
    uint32_t *ptr = data;
    if(cnt == 0)
        cnt = *ptr;
    for(uint32_t i = 0; i < len; i+=4){
        if(cnt != *ptr) {
            printf("net eq, %u, 0x%x[%u], 0x%x[%u], index：%d, len=%u\n", i, cnt, cnt,  *ptr, *ptr, index, len);
            //exit(1);
            if(k ++ > 10)
                exit(1);
        } else{
            //printf("%d, %x\n", i, cnt);
        }
        ptr++;
        cnt += 4;
    }
    
    return true;
}

static int xdma_data_dispatcher_buffer(int ch, void **data, uint32_t *len, ssize_t count, void *args)
{
    struct net_sub_st sub;
    ssize_t offset = 0, nframe = 0;
    uint8_t *ptr = NULL;
    struct spm_run_parm *run = args; 
    
    for(int index = 0; index < count; index++){
        offset = 0;
        ptr = data[index];
        printf_info(">>>>>index=%d, %p, %ld\n", index, ptr, count);
        do{
 #ifdef SPM_HEADER_CHECK
            nframe = _xdma_of_match_pkgs(ptr, len[index], &sub, offset);
            //nframe = _xdma_of_match_pkgs_test(ptr, len[index], &sub, offset);
 #else
            nframe = len[index];
            offset = XDMA_BLOCK_SIZE+1;
            sub.chip_id = 0x0502;
            sub.func_id = 0x0;
            _data_check(ptr, len[index], index);
 #endif
            if(nframe <= 0){
                printf_note(">>>>>read block[%d/%ld] over, size[%ld]\n", index+1, count, offset);
                break;
            }
            ptr += nframe;
            offset += nframe;
            printf_info("offset：%ld, chip_id=0x%x, func_id=0x%x\n", offset, sub.chip_id, sub.func_id);
            _xdma_load_disp_buffer(data[index], nframe, &sub, args);
        }while(offset < XDMA_BLOCK_SIZE);
        run->xdma_disp.inout.read_bytes += len[index];
    }
    return 0;
}


static int xdma_data_dispatcher(int ch, void **data, uint32_t *len, ssize_t count, void *args)
{
/*
51 57 0f 30 18 01 00 00 00 00 02 05 00 00 08 01 02 02 00 00 00 00 00 00 aa aa 55 55 63 02 00 00 2f 05 74 09 36 05 c5 09 00 00（上行）
|---------------------|  | a|  | b|  | c|  | d|  | e|  | f|  | g|

a:物理目的地址
b:物理源地址
c:目的地址(目的组件/连接器地址)
d:载荷长度
e:分段表示，消息类型，请求表示
f:源组件地址
g:命令表示
*/

    struct net_sub_st parg;
    struct spm_run_parm *arg = args; 

    struct data_frame_st{
        uint16_t py_dist_addr;
        uint16_t py_src_addr;
        uint16_t dist_addr;
        uint16_t len;
        uint16_t type;
        uint16_t src_addr;
        uint16_t cmd;
    }__attribute__ ((packed));
    struct data_frame_st *pdata;
    
    if(args == NULL){
        return -1;
    }
    
    uint8_t *ptr = NULL;
    uint8_t *payload = NULL;
    int hashid = 0,vec_cnt = 0,flen = 0;

    for(int index = 0; index < count; index++){
        ptr = data[index];
        if(ptr == NULL){
            printf_err("ptr is null\n");
            continue;
        }
#ifdef SPM_HEADER_CHECK
        if(*(uint16_t *)ptr != 0x5751){  /* header */
            printf_warn("error header=0x%x\n", *(uint16_t *)ptr);
            return -1;
        }
        flen = *(uint16_t *)(ptr + 4);
        if(flen > len[index])
            printf_note("frame len: [0x%x]%d, %d[0x%x]\n", flen,flen, len[index],len[index]);
#endif
        payload = ptr + 8;
#ifdef SPM_HEADER_CHECK
        pdata = (struct data_frame_st *)payload;
        parg.chip_id = pdata->py_src_addr;
        parg.func_id = pdata->src_addr;
#else
        parg.chip_id = 0x0502;
        parg.func_id = 0x0;
#endif
        hashid = GET_HASHMAP_ID(parg.chip_id, parg.func_id, parg.prio_id, parg.port);
       // hashid = find_hash_id(parg.chip_id, parg.func_id);
        printf_note("hashid:%d, vec_cnt=%d,chip_id=%x, func_id=0x%x, prio_id=0x%x\n", hashid, vec_cnt,
                    parg.chip_id, parg.func_id, parg.prio_id);
        if(hashid > MAX_XDMA_DISP_TYPE_NUM || hashid < 0 || arg->xdma_disp.type[hashid] == NULL){
            printf_err("hash id err[%d]\n", hashid);
            continue;
        }
        vec_cnt = arg->xdma_disp.type[hashid]->vec_cnt;
        arg->xdma_disp.type[hashid]->subinfo.chip_id = parg.chip_id;
        arg->xdma_disp.type[hashid]->subinfo.func_id = parg.func_id;
        arg->xdma_disp.type[hashid]->vec[vec_cnt].iov_base = data[index];
        arg->xdma_disp.type[hashid]->vec[vec_cnt].iov_len = len[index];
        //printf_note("iov_base=%p, iov_len=%lu\n", 
        //    arg->xdma_disp.type[hashid]->vec[vec_cnt].iov_base, arg->xdma_disp.type[hashid]->vec[vec_cnt].iov_len);
        arg->xdma_disp.type_num++;
        arg->xdma_disp.type[hashid]->vec_cnt++;
        //printf_note("offset=%d, chip_id=0x%x, func_id=0x%x,hashid=%d, type_num=%d\n", offset, parg.chip_id, parg.func_id, hashid,  arg->xdma_disp.type_num);
    }

    return 0;
}

static ssize_t xspm_read_xdma_data_dispatcher(int ch , void **data, uint32_t *len, void *args)
{
    int index;
    ssize_t count = 0;
    
    index = xspm_find_index_by_rw(ch, -1, XDMA_READ);
    if(index < 0)
        return -1;

    count = xspm_stream_read(ch, index, data, len, args);
   // count = xspm_stream_read_test2(ch, index, data, len, args);
    //count = xspm_stream_read_test(ch, index, data, len, args);
    if(count > 0 && count < XDMA_TRANSFER_MAX_DESC){
        if(xdma_data_dispatcher_buffer(ch, data, len, count, args) == -1){
            return -1;
        }
    }
    return count;
}

static int xspm_send_data_dispatcher(int ch, char *buf[], uint32_t len[], int count, void *args)
{
    struct spm_run_parm *arg = args; 
    struct net_sub_st *sub;
    struct iovec iov[count];

    for(int i = 0; i < MAX_XDMA_DISP_TYPE_NUM; i++){
        if(arg->xdma_disp.type[i] == NULL || arg->xdma_disp.type[i]->vec_cnt == 0)
            continue;
        
        sub = &arg->xdma_disp.type[i]->subinfo;
        if(arg->xdma_disp.type[i]->vec_cnt > count){
            printf_err("type vec_cnt err:%d, %d\n", arg->xdma_disp.type[i]->vec_cnt, count);
            break;
        }
        for(int j = 0; j < arg->xdma_disp.type[i]->vec_cnt; j++){
            iov[j].iov_base = arg->xdma_disp.type[i]->vec[j].iov_base;
            iov[j].iov_len =  arg->xdma_disp.type[i]->vec[j].iov_len;
            //printf_note("[%d]iov=%p, %lu, vec_cnt=%d\n", j, iov[j].iov_base, iov[j].iov_len, arg->xdma_disp.type[i]->vec_cnt);
        }
        if(arg->xdma_disp.type[i]->vec_cnt > 0){
            //printf_note("tcp send: %d, %x\n", arg->xdma_disp.type[i]->vec_cnt, sub->chip_id);
            tcp_send_vec_data_uplink(iov, arg->xdma_disp.type[i]->vec_cnt, sub);
        }
    }
    xdma_data_dispatcher_refresh(ch, args);

    return 0;
}

static int xspm_send_data(int ch, char *buf[], uint32_t len[], int count, void *args)
{
    int section_id = *(int *)args;
    struct iovec iov[count];

    //printf_note("count:%d, %p,%p, %u\n", count, buf[0],buf[0]+0xf01,  len[0]);
    for(int i = 0; i < count; i++){
        iov[i].iov_base = buf[i];
        iov[i].iov_len =  len[i];
        //printf_note("base: %p, %lu\n", iov[i].iov_base, iov[i].iov_len);
        //tcp_send_data_uplink(buf[i], len[i], args);
    }
    tcp_send_vec_data_uplink(iov, count, args);
    xspm_read_xdma_data_over(ch, NULL);
    
    return 0;
}


static int xspm_read_stream_start(int ch, int subch, uint32_t len,uint8_t continuous, enum stream_type type)
{
    int index, reg, rc;
    struct _spm_xstream *pstream = spm_xstream;
    struct xdma_ring_trans_ioctl ring_trans;
    
    index = xspm_find_index_by_rw(ch, -1, XDMA_READ);
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
    struct _spm_xstream *pstream = spm_xstream;
    int index, ret, reg;
    struct xdma_ring_trans_ioctl ring_trans;

    index = xspm_find_index_by_rw(ch, -1, XDMA_READ);
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
    	printf_note("ioctl(IOCTL_XDMA_PERF_STOP) error:%d\n", ret);
    }
    #endif

    printf_debug("stream_stop: %d, %s\n", index, pstream[index].name);
    return 0;
}

static int _xspm_close(void *_ctx)
{
    struct spm_context *ctx = _ctx;
    struct _spm_xstream *pstream = spm_xstream;
    int i, ch;

    for(i = 0; i< ARRAY_SIZE(spm_xstream) ; i++){
            if(pstream[i].rd_wr == XDMA_READ)
                xspm_read_stream_stop(-1, -1, XDMA_STREAM);
        close(pstream[i].id);
    }
    for(ch = 0; ch< MAX_RADIO_CHANNEL_NUM; ch++){
        //safe_free(ctx->run_args[ch]->fft_ptr);
       // safe_free(ctx->run_args[ch]);
    }
    close_file();
    return 0;
}

static int xspm_xdma_data_clear(int ch,  void *arg)
{
    struct spm_run_parm *run = arg;
    xspm_read_stream_stop(0, 0, XDMA_STREAM);
    xspm_read_stream_start(0, 0, 0, 0, XDMA_STREAM);
    for(int i = 0; i < MAX_XDMA_DISP_TYPE_NUM; i++){
        run->xdma_disp.type[i]->vec_cnt = 0;
    }
    return 0;
}


static int xspm_read_xdma_data_over(int ch,  void *arg)
{
    int ret;
    struct _spm_xstream *pstream = spm_xstream;
    
    int index;

    //index = xspm_find_index_by_type(ch, -1, XDMA_STREAM);
    index = xspm_find_index_by_rw(ch, -1, XDMA_READ);
    if(index < 0)
        return -1;
    
    struct xdma_ring_trans_ioctl *ring_trans = &xinfo[index];
    #if 0
   static  struct xdma_ring_trans_ioctl ring_trans;

    printf_note("id =%d, index=%d, consume_index=%d\n", pstream[index].id, index, pstream[index].consume_index);
    memset(&ring_trans, 0, sizeof(ring_trans));
    ring_trans.block_size = pstream[index].block_size;
    ring_trans.rx_index = pstream[index].consume_index;
//    ring_trans.ready_count = pstream[index].block_size;
    ring_trans.invalid_index = pstream[index].consume_index;
    ring_trans.invalid_count = 1;
    #endif
    //struct xdma_ring_trans_ioctl *ring_trans = arg;
    printf_debug("*OVER index=%d block_size=%u,block_count=%u,ready_count=%u,rx_index=%u\n", index,
        ring_trans->block_size, ring_trans->block_count, ring_trans->ready_count, ring_trans->rx_index);
    ring_trans->invalid_index = ring_trans->rx_index;
    ring_trans->invalid_count = ring_trans->ready_count;
    if(pstream[index].id < 0)
        return -1;

    if(pstream){
        ret = ioctl(pstream[index].id, IOCTL_XDMA_TRANS_SET, ring_trans);
        if (ret){
           // printf("ioctl(IOCTL_XDMA_TRANS_SET) failed:%d\n", ret);
            return -1;
        }
    }
    return 0;
}


static const struct spm_backend_ops xspm_ops = {
    .create = xspm_create,
#ifdef SPM_DISPATCHER_ON
    .read_xdma_data = xspm_read_xdma_data_dispatcher,
    .send_xdma_data = xspm_send_data_dispatcher,
#else
    .read_xdma_data = xspm_read_xdma_data,
    .send_xdma_data = xspm_send_data,
#endif
    .read_xdma_over_deal = xspm_read_xdma_data_over,
    .stream_start = xspm_read_stream_start,
    .stream_stop = xspm_read_stream_stop,
    .write_xdma_data = xspm_stram_write,
    .close = _xspm_close,
};

struct spm_context * spm_create_xdma_context(void)
{
    int ret = -ENOMEM, ch;
    unsigned int len;
    struct spm_context *ctx = zalloc(sizeof(*ctx));
    if (!ctx)
        goto err_set_errno;

    ctx->ops = &xspm_ops;
    ctx->pdata = &config_get_config()->oal_config;
    _init_run_timer(MAX_XDMA_NUM);
    printf_note("create xdma ctx\n");
err_set_errno:
    errno = -ret;
    return ctx;

}


