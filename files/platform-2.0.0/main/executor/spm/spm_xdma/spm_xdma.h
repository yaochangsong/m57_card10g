#ifndef _SPM_XDMA_H
#define _SPM_XDMA_H

#define DMA_BUFFER_16M_SIZE (16 * 1024 * 1024)
#define DMA_BUFFER_32M_SIZE (32 * 1024 * 1024)
#define DMA_BUFFER_64M_SIZE (64 * 1024 * 1024)
#define DMA_BUFFER_128M_SIZE (128 * 1024 * 1024)
#define DMA_BUFFER_1G_SIZE (0x40000000)
#define DMA_BUFFER_256M_SIZE (0x10000000U)
#define DMA_BUFFER_512M_SIZE (0x20000000U)


#define XDMA_BUFFER_SIZE DMA_BUFFER_512M_SIZE
#define XDMA_BLOCK_SIZE (0x400000)//(0x100000)//(0x400000)
#define XDMA_TRANSFER_MAX_DESC (2048)
#define XDMA_DATA_TYPE_MAX_PKGS (65536)


typedef struct _spm_xstream {
    spm_base_stream_t base;
    uint32_t block_size;
    int consume_index;  /* 每次消费的块索引 */
    uint8_t *ptr_vec[XDMA_TRANSFER_MAX_DESC];       /* 频谱流数据buffer指针 */
}spm_xstream_t;


/*bob 20191017 for C2H revice ring buffer function*/
enum xdma_ring_trans_state {
	RING_TRANS_OK = 0,			//返回有效的数据块
	RING_TRANS_INITIALIZING,    //DMA初始化中
	RING_TRANS_FAILED,          //DMA错误
	RING_TRANS_OVERRUN,         //DMA缓存已满，有数据将被覆盖
	RING_TRANS_PENDING          //DMA运行中，但没有接收到数据
};

enum xdma_status {
	XDMA_STATUS_STOP = 0,	//DMA停止状态
	XDMA_STATUS_BUSY,		//DMA工作忙状态
	XDMA_STATUS_IDLE,		//DMA空闲状态
	XDMA_STATUS_ERROR       //DMA出错误状态
};

struct ring_xdma_result 
{
	uint32_t status;
	uint32_t length;
} __packed;

struct xdma_ring_trans_ioctl
{
    uint32_t version;
    uint32_t block_size;
    uint32_t rx_index;
    uint32_t invalid_index;
    uint32_t ready_count;
    uint32_t free_count;
    uint32_t invalid_count;
    uint32_t pending_count;
    uint32_t block_count;
    enum xdma_ring_trans_state status;  //defined from xdma_ring_trans_state
    struct ring_xdma_result results[XDMA_TRANSFER_MAX_DESC];
};
/*bob 20191017*/

/* IOCTL codes */

#define IOCTL_XDMA_PERF_START   _IOW('q', 1, struct xdma_performance_ioctl *)
#define IOCTL_XDMA_PERF_STOP    _IOW('q', 2, struct xdma_performance_ioctl *)
#define IOCTL_XDMA_PERF_GET     _IOR('q', 3, struct xdma_performance_ioctl *)
#define IOCTL_XDMA_ADDRMODE_SET _IOW('q', 4, int)
#define IOCTL_XDMA_ADDRMODE_GET _IOR('q', 5, int)
#define IOCTL_XDMA_ALIGN_GET    _IOR('q', 6, int)

/*bob 20191017 for C2H revice ring buffer function*/
#define IOCTL_XDMA_TRANS_START   _IOW('q', 7, struct xdma_ring_trans_ioctl *)
#define IOCTL_XDMA_TRANS_STOP    _IOW('q', 8, struct xdma_ring_trans_ioctl *)
#define IOCTL_XDMA_TRANS_GET     _IOR('q', 9, struct xdma_ring_trans_ioctl *)
#define IOCTL_XDMA_TRANS_SET	 _IOW('q', 10, struct xdma_ring_trans_ioctl *)
#define IOCTL_XDMA_STATUS	 _IOW('q', 11, struct xdma_ring_trans_ioctl *)
#define IOCTL_XDMA_INIT_BUFF	 _IOW('q', 12, struct xdma_ring_trans_ioctl *)
/*bob 20191017*/

extern struct spm_context * spm_create_context(void);

#endif

