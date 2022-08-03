#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>
#include <pthread.h>
#include "statistics.h"
#include "config.h"


spm_statistics_t *spm_stats;

static uint64_t _get_speed(int index, int time_sec, uint64_t data)
{
    static uint64_t olddata[16] = {[0 ... 15] = 0};
    uint64_t speed = 0;

    if(data > olddata[index]){
        speed = (data - olddata[index]) / time_sec;
    }
    olddata[index] = data;
    
    return speed;
}


void *_statistics_speed_loop(void *args)
{
    spm_statistics_t *stats = args;
    int time_interval = 3;
    int ch = 0;
    pthread_detach(pthread_self());
    void *stream = spm_dev_get_stream(&ch);

    if(!stats || ch <= 0 || !stream)
        pthread_exit(0);
    if(ch > DMA_CHANNEL_STATS){
        ch = DMA_CHANNEL_STATS;
    }
    while(1){
        sleep(time_interval);
        for(int i = 0; i < ch; i++){
            stats->dma_stats[i].read_speed_bps = _get_speed(0, time_interval, stats->dma_stats[i].read_bytes);
            stats->dma_stats[i].send_speed_bps = _get_speed(1, time_interval, stats->dma_stats[i].send_bytes);
           // printf_note("dma%d read speed: %ubps, %"PRIu64"\n", i, stats->dma_stats[i].read_speed_bps, stats->dma_stats[i].read_bytes);
        }
    }
}


int statistics_speed_thread(void *args)
{
    int ret;
    pthread_t tid;
    ret=pthread_create(&tid, NULL, _statistics_speed_loop, args);
    if(ret!=0){
        perror("pthread create");
        return -1;
    }
    return 0;
}


void statistics_init(void)
{
    spm_statistics_t *s = NULL;
    s = calloc(1, sizeof(*s));
    if(!s)
        return;
    printf_note("Statistics Init\n");
    spm_stats = s;
    statistics_speed_thread(s);
}
