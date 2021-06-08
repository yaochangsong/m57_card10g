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


static int xspm_stream_stop(int ch, int subch, enum stream_type type);

static inline void *zalloc(size_t size)
{
	return calloc(1, size);
}

static bool is_client_equal(struct net_tcp_client *cl, struct net_tcp_client *dst_cl)
{
    if(cl->peer_addr.sin_addr.s_addr == dst_cl->peer_addr.sin_addr.s_addr && 
        cl->peer_addr.sin_port == dst_cl->peer_addr.sin_port){
        return true;
    }
    return false;
}


static struct _spm_xstream spm_xstream[] = {
        {XDMA_R_DEV0,        -1, 0, NULL, XDMA_BUFFER_SIZE, XDMA_BLOCK_SIZE, "XDMA Stream0", XDMA_READ, XDMA_STREAM, -1},
        {XDMA_R_DEV1,        -1, 1, NULL, XDMA_BUFFER_SIZE, XDMA_BLOCK_SIZE, "XDMA Stream1", XDMA_READ, XDMA_STREAM, -1},
//        {XDMA_R_DEV2,        -1, 2, NULL, XDMA_BUFFER_SIZE, XDMA_BLOCK_SIZE, "XDMA Stream2", XDMA_READ, XDMA_STREAM, -1},
//        {XDMA_R_DEV3,        -1, 3, NULL, XDMA_BUFFER_SIZE, XDMA_BLOCK_SIZE, "XDMA Stream3", XDMA_READ, XDMA_STREAM, -1},
};

int _WaitDeviceReady(int dev_fd)
{
    #define DEVICE_READY_TIMEOUT 1000  //unit is ms
	int ret = 0;
	int isReady = 0;
	int timeout = DEVICE_READY_TIMEOUT;
	struct xdma_ring_trans_ioctl ring_trans;
	memset(&ring_trans, 0, sizeof(struct xdma_ring_trans_ioctl));

	while(timeout--)
	{
		ret = ioctl(dev_fd, IOCTL_XDMA_TRANS_GET, &ring_trans);
		if (ret) 
		{
			printf("ioctl(IOCTL_XDMA_TRANS_GET) failed %d\n", ret);
			return -1;	  
		}	
		
		if(ring_trans.status != RING_TRANS_INITIALIZING)
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
    printf_note("SPM init.%ld\n", ARRAY_SIZE(spm_xstream));
    
    /* create stream */
    for(i = 0; i< ARRAY_SIZE(spm_xstream) ; i++){
        pstream[i].id = open(pstream[i].devname, O_RDWR);
        if( pstream[i].id < 0){
            fprintf(stderr, "[%d]open:%s, %s\n", i, pstream[i].devname, strerror(errno));
            continue;
        }

        pstream[i].ptr = mmap(NULL, pstream[i].len, PROT_READ | PROT_WRITE,MAP_SHARED, pstream[i].id, 0);
        if (pstream[i].ptr == (void*) -1) {
            fprintf(stderr, "mmap: %s\n", strerror(errno));
            exit(-1);
        }
        printf_warn("[%d, ch=%d]create stream[%s]: dev:%s, ptr=%p, len=%d\n", pstream[i].id,pstream[i].ch,
            pstream[i].name, pstream[i].devname, pstream[i].ptr, pstream[i].len);
        
        memset(&ring_trans, 0, sizeof(struct xdma_ring_trans_ioctl));
        ring_trans.block_size = pstream[i].block_size;
        ring_trans.block_count = pstream[i].len / pstream[i].block_size;
        io_xdma_set_speed(0);
        io_xdma_force_ready(pstream[i].ch, true);
        xspm_stream_stop(pstream[i].ch, 0, XDMA_STREAM);
        usleep(1000);
    }
    
    return 0;
}


struct xdma_ring_trans_ioctl xinfo[4];

static ssize_t xspm_stream_read(int type,  void **data, void *args)
{
    int rc;
    struct _spm_xstream *pstream;
    pstream = spm_xstream;
    struct xdma_ring_trans_ioctl *info = &xinfo[type];
    size_t readn = 0;
    
    memset(info, 0, sizeof(struct xdma_ring_trans_ioctl));
    
    if(pstream[type].id < 0){
        printf_warn("%d stream node:%s not found\n",type, pstream[type].name);
        return -1;
    }
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
        } else if(info->status == RING_TRANS_INITIALIZING){
            printf_warn("*****status:RING_TRANS_INITIALIZING.*****\n");
            usleep(10);
        } else if(info->status == RING_TRANS_PENDING)
            usleep(1);
    }while(info->status == RING_TRANS_PENDING);
    readn = info->block_size;
    *data = pstream[type].ptr + info->rx_index*readn;
    pstream[type].consume_index = info->rx_index;
   // if(args != NULL)
   //     memcpy(args, info, sizeof(info));

    printf_debug("[%p, %p, offset=0x%x]%s, readn:%lu\n", *data, pstream[type].ptr, info->rx_index,  pstream[type].name, readn);

    return readn;
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

int xspm_get_dma_fd(int ch)
{
    int index;
    struct _spm_xstream *pstream;
    pstream = spm_xstream;

    index = xspm_find_index_by_type(ch, -1, XDMA_STREAM);
    if(index < 0)
        return -1;

    return pstream[index].id;
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
    
    index = xspm_find_index_by_type(ch, -1, XDMA_STREAM);
    if(index < 0)
        return -1;
    
    if(pstream[index].id < 0)
        return -1;

    while (buflen) {
        len =  write(pstream[index].id, buf, buflen);
        if (len < 0) {
            printf_note("[fd:%d]-send len : %ld, %d[%s]\n", pstream[index].id, len, errno, strerror(errno));
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


static ssize_t xspm_read_xdma_data(int ch, void **data, void *args)
{
    int index;

    index = xspm_find_index_by_type(ch, -1, XDMA_STREAM);
    if(index < 0)
        return -1;
    //printf_warn("index=%d, ch=%d\n", index, ch);
    return xspm_stream_read(index, data, args);
}

static int xspm_send_data(int ch, const char *buf, int len, void *args)
{

    struct iovec iov[2];

    iov[0].iov_base = buf;
    iov[0].iov_len = len;

#if (defined SUPPORT_DATA_PROTOCAL_TCP)
    tcp_send_vec_data(iov, 1, NET_DATA_TYPE_XDMA);
#else
    udp_send_vec_data(iov, 1, NET_DATA_TYPE_XDMA);
#endif
    #if 0
    struct net_tcp_client  *cl = tcp_get_datasrv_client(ch);
    int r;
    
    if(cl == NULL)
        return -1;

    r = cl->send_raw(cl, buf, buflen);

    printf_debug("ch=%d, r = %d, buflen = %d\n", ch, r, buflen);
    #endif
    return 0;
}

static ssize_t xspm_read_xdma_data_test(int ch, void **data)
{   
    static int count = 0;
    static char buffer[4096*8] = {0};
    int index;
    
    index = xspm_find_index_by_type(ch, -1, XDMA_STREAM);
    if(index < 0)
        return -1;
    
    for(int i = 0; i< sizeof(buffer); i++){
        buffer[i] = i;
    }
    //printf_note("%d , %d, buffer=%p\n", buffer[0], buffer[1], buffer);
    *data = buffer;
    
   // if(count++ >= 1)
   //     return -1;
    return sizeof(buffer);
    //return xspm_stream_read(index, data);
}



static int xspm_stream_start(int ch, int subch, uint32_t len,uint8_t continuous, enum stream_type type)
{
    int index, reg, rc;
    struct _spm_xstream *pstream = spm_xstream;
    struct xdma_ring_trans_ioctl ring_trans;
    
    index = xspm_find_index_by_type(ch, subch, type);
    if(index < 0)
        return -1;
    
    memset(&ring_trans, 0, sizeof(struct xdma_ring_trans_ioctl));
    ring_trans.block_size = pstream[index].block_size;
    ring_trans.block_count = pstream[index].len / pstream[index].block_size;

    if(pstream[index].id < 0)
        return -1;
    
    rc = ioctl(pstream[index].id, IOCTL_XDMA_TRANS_START, &ring_trans);
    if (rc == 0) {
        printf("IOCTL_XDMA_TRANS_START succesful.\n");
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

    io_xdma_enable(ch);
    return 0;
}

static int xspm_stream_stop(int ch, int subch, enum stream_type type)
{
    struct _spm_xstream *pstream = spm_xstream;
    int index, ret, reg;
    struct xdma_ring_trans_ioctl ring_trans;
    
    index = xspm_find_index_by_type(ch, subch, type);
    if(index < 0)
        return -1;
    printf_note("ch=%d, type=%d, id=%d\n", ch, type, pstream[index].id);

    #if 1
    if(pstream[index].id < 0)
        return -1;
    ret = ioctl(pstream[index].id, IOCTL_XDMA_TRANS_STOP, &ring_trans);
    if (ret == 0) {
    	printf_note("IOCTL_XDMA_PERF_STOP succesful.\n");
    } 
    else {
    	printf_note("ioctl(IOCTL_XDMA_PERF_STOP) error:%d\n", ret);
    }
    #endif
    io_xdma_disable(ch);

    printf_debug("stream_stop: %d, %s\n", index, pstream[index].name);
    return 0;
}

static int _xspm_close(void *_ctx)
{
    struct spm_context *ctx = _ctx;
    struct _spm_xstream *pstream = spm_xstream;
    int i, ch;

    for(i = 0; i< ARRAY_SIZE(spm_xstream) ; i++){
        xspm_stream_stop(-1, -1, i);
        close(pstream[i].id);
    }
    for(ch = 0; ch< MAX_RADIO_CHANNEL_NUM; ch++){
        safe_free(ctx->run_args[ch]->fft_ptr);
        safe_free(ctx->run_args[ch]);
    }
    printf_note("close..\n");
    return 0;
}

static int xspm_read_xdma_data_over(int ch,  void *arg)
{
    int ret;
    struct _spm_xstream *pstream = spm_xstream;
    
    int index;

    index = xspm_find_index_by_type(ch, -1, XDMA_STREAM);
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
    printf_note("*OVER index=%d block_size=%u,block_count=%u,ready_count=%u,rx_index=%u\n", index,
        ring_trans->block_size, ring_trans->block_count, ring_trans->ready_count, ring_trans->rx_index);
    ring_trans->invalid_index = ring_trans->rx_index;
    ring_trans->invalid_count = 1;//ring_trans->ready_count;
    if(pstream[index].id < 0)
        return -1;

    if(pstream){
        ret = ioctl(pstream[index].id, IOCTL_XDMA_TRANS_SET, ring_trans);
        if (ret){
            printf("ioctl(IOCTL_XDMA_TRANS_SET) failed:%d\n", ret);
            return -1;
        }
    }
    return 0;
}

static int xspm_read_xdma_data_over_test(int ch,  void *arg)
{
    printf_note("xdma read over!\n");
    return 0;
}


static const struct spm_backend_ops xspm_ops = {
    .create = xspm_create,
    //.read_xdma_data = xspm_read_xdma_data,//xspm_read_xdma_data_test, //
    .read_xdma_data = xspm_read_xdma_data,
    .read_xdma_over_deal = xspm_read_xdma_data_over,// xspm_read_xdma_data_over_test,//xspm_read_xdma_data_over,
    .stream_start = xspm_stream_start,
    .stream_stop = xspm_stream_stop,
    .write_xdma_data = xspm_stram_write,
    //.data_dispatcher = xspm_data_dispatcher,
    .send_xdma_data = xspm_send_data,
    //.send_dispatcher = xspm_dispatcher_send,
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

err_set_errno:
    errno = -ret;
    return ctx;

}


