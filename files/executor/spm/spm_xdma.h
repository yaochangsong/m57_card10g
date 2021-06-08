#ifndef _SPM_XDMA_H
#define _SPM_XDMA_H

#define XDMA_R_DEV0 "/dev/xdma0_c2h_0"
#define XDMA_R_DEV1 "/dev/xdma0_c2h_1"
#define XDMA_R_DEV2 "/dev/xdma0_c2h_2"
#define XDMA_R_DEV3 "/dev/xdma0_c2h_3"




#define DMA_BUFFER_16M_SIZE (16 * 1024 * 1024)
#define DMA_BUFFER_64M_SIZE (64 * 1024 * 1024)
#define DMA_BUFFER_128M_SIZE (128 * 1024 * 1024)
#define DMA_BUFFER_1G_SIZE (0x40000000)
#define DMA_BUFFER_256M_SIZE (0x10000000)

#define XDMA_BUFFER_SIZE DMA_BUFFER_256M_SIZE
#define XDMA_BLOCK_SIZE 8388608


struct _spm_xstream {
    char *devname;      /* 流设备节点名称 */
    int id;             /* 频谱流类型描述符 */
    int ch;
    uint8_t *ptr;       /* 频谱流数据buffer指针 */
    uint32_t len;       /* 频谱流数据buffer长度 */
    uint32_t block_size;
    char *name;         /* 频谱流名称 */
    int rd_wr;
    enum stream_type type;
    int consume_index;  /* 每次消费的块索引 */
};

typedef enum _xdma_tx_rx_
{
    XDMA_READ = 0,
    XDMA_WRITE = 1,
}xdma_tx_rx;

/*bob 20191017 for C2H revice ring buffer function*/
enum xdma_ring_trans_state {
	RING_TRANS_OK = 0,			//返回有效的数据块
	RING_TRANS_INITIALIZING,    //DMA初始化中
	RING_TRANS_FAILED,          //DMA错误
	RING_TRANS_OVERRUN,         //DMA缓存已满，有数据将被覆盖
	RING_TRANS_PENDING          //DMA运行中，但没有接收到数据
};

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
/*bob 20191017*/


struct spm_data_node{
    struct list_head normal_list;
    struct list_head urgent_list;
    uint8_t *data;
    uint32_t data_len;
};
struct spm_dispatcher{
    #define _DATA_CARDID_NUM_MAX 4
    struct list_head normal_list[_DATA_CARDID_NUM_MAX];
    struct list_head urgent_list[_DATA_CARDID_NUM_MAX];
    uint32_t normal_num[_DATA_CARDID_NUM_MAX];
    uint32_t urgent_num[_DATA_CARDID_NUM_MAX];
};

extern struct spm_context * spm_create_xdma_context(void);
extern int xspm_get_dma_fd(int ch);
extern void *xspm_dispatcher_init(void);

#endif

