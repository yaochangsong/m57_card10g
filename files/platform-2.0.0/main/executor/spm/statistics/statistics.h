#ifndef STATISTICS_H_H
#define STATISTICS_H_H

/* dma通道频谱数据统计 */
typedef struct dma_channel_statistics{
    volatile uint64_t read_bytes;           /* 读取字节数 */
    volatile uint64_t read_pkts;            /* 读取包(块)数 */
    volatile uint64_t send_bytes;           /* 准备发送字节数 */
    volatile uint64_t write_bytes;          /* 写入字节数 */
    volatile uint32_t read_speed_bps;       /* 读取字节速度bps */
    volatile uint32_t send_speed_bps;       /* 发送字节速度bps */
    volatile uint64_t over_run_count;       /* 溢出次数 */
    volatile uint64_t fpga_over_run_count;  /* fpga溢出次数 */
}dma_channel_statistics_t;

/* 上行数据统计 */
typedef struct uplink_statistics{
    volatile uint64_t send_ok_bytes;        /* 成功发送字节数 */
    volatile uint32_t send_speed_bps;       /* 发送字节速度bps */
    volatile uint32_t rcv;
}uplink_statistics_t;

/* 下行数据统计 */
typedef struct downlink_statistics{
    volatile uint64_t cmd_pkt;              /* 成功解析命令包 */
    volatile uint32_t err_pkt;              /* 解析错误命令包*/
    volatile uint32_t rcv;
}downlink_statistics_t;

#define DMA_CHANNEL_STATS 8
typedef struct spm_statistics{
    dma_channel_statistics_t dma_stats[DMA_CHANNEL_STATS];
    uplink_statistics_t uplink;
    downlink_statistics_t downlink;
}spm_statistics_t;

extern spm_statistics_t *spm_stats;

static inline void update_uplink_send_ok_bytes(size_t len)
{
    if(!spm_stats)
        return;
    spm_stats->uplink.send_ok_bytes += len;
}

static inline uint64_t get_uplink_send_ok_bytes(void)
{
    if(!spm_stats)
        return 0;

    return spm_stats->uplink.send_ok_bytes;
}

static inline void update_downlink_cmd_pkt(size_t len)
{
    if(!spm_stats)
        return;

    spm_stats->downlink.cmd_pkt += len;
}

static inline uint64_t get_downlink_cmd_pkt(void)
{
    if(!spm_stats)
        return 0;

    return spm_stats->downlink.cmd_pkt;
}

static inline void update_downlink_cmd_err_pkt(size_t len)
{
    if(!spm_stats)
        return;

    spm_stats->downlink.err_pkt += len;
}

static inline uint64_t get_downlink_cmd_err_pkt(void)
{
    if(!spm_stats)
        return 0;

    return spm_stats->downlink.err_pkt;
}


static inline void update_dma_readbytes(int ch, size_t len)
{
    if(!spm_stats || ch >= DMA_CHANNEL_STATS || ch < 0)
        return;
    spm_stats->dma_stats[ch].read_bytes += len;
}

static inline uint64_t get_dma_readbytes(int ch)
{
    if(!spm_stats || ch >= DMA_CHANNEL_STATS || ch < 0)
        return 0;

    return spm_stats->dma_stats[ch].read_bytes ;
}

static inline void update_dma_write_bytes(int ch, size_t len)
{
    if(!spm_stats || ch >= DMA_CHANNEL_STATS || ch < 0)
        return;

    spm_stats->dma_stats[ch].write_bytes += len;
}

static inline uint64_t get_dma_write_bytes(int ch)
{
    if(!spm_stats || ch >= DMA_CHANNEL_STATS || ch < 0)
        return 0;

    return spm_stats->dma_stats[ch].write_bytes ;
}


static inline void update_dma_read_pkts(int ch, size_t len)
{
    if(!spm_stats || ch >= DMA_CHANNEL_STATS || ch < 0)
        return;

    spm_stats->dma_stats[ch].read_pkts += len;
}

static inline uint64_t get_dma_read_pkts(int ch)
{
    if(!spm_stats || ch >= DMA_CHANNEL_STATS || ch < 0)
        return 0;

    return spm_stats->dma_stats[ch].read_pkts;
}

static inline uint64_t get_dma_read_speed(int ch)
{
    if(!spm_stats || ch >= DMA_CHANNEL_STATS || ch < 0)
        return 0;

    return spm_stats->dma_stats[ch].read_speed_bps;
}

static inline uint64_t get_dma_send_speed(int ch)
{
    if(!spm_stats || ch >= DMA_CHANNEL_STATS || ch < 0)
        return 0;

    return spm_stats->dma_stats[ch].send_speed_bps;
}


static inline void update_dma_send_bytes(int ch, size_t len)
{
    if(!spm_stats || ch >= DMA_CHANNEL_STATS || ch < 0)
        return;

    spm_stats->dma_stats[ch].send_bytes += len;
}

static inline uint64_t get_dma_send_bytes(int ch)
{
    if(!spm_stats || ch >= DMA_CHANNEL_STATS || ch < 0)
        return 0;

    return spm_stats->dma_stats[ch].send_bytes;
}


static inline void update_dma_over_run_count(int ch, size_t len)
{
    if(!spm_stats || ch >= DMA_CHANNEL_STATS || ch < 0)
        return;

    spm_stats->dma_stats[ch].over_run_count += len;
}

static inline uint64_t get_dma_over_run_count(int ch)
{
    if(!spm_stats || ch >= DMA_CHANNEL_STATS || ch < 0)
        return 0;

    return spm_stats->dma_stats[ch].over_run_count;
}


extern void statistics_init(void);

#endif
