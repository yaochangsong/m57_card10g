/******************************************************************************
*  Copyright 2022, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2022
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   26 July. 2022   yaochangsong
*  Initial revision.
******************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "file_sink.h"

static FILE *fp[FILE_SINK_TYPE_MAX] = {[0 ... FILE_SINK_TYPE_MAX-1] = NULL};
static struct timeval start[FILE_SINK_TYPE_MAX];
static struct timeval now[FILE_SINK_TYPE_MAX];
static int file_sink_time_ms[FILE_SINK_TYPE_MAX] = {[0 ... FILE_SINK_TYPE_MAX-1] = 0};


static void _gettime(struct timeval *tv)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    tv->tv_sec = ts.tv_sec;
    tv->tv_usec = ts.tv_nsec / 1000;
}

static int _tv_diff(struct timeval *t1, struct timeval *t2)
{
    return
        (t1->tv_sec - t2->tv_sec) * 1000 +
        (t1->tv_usec - t2->tv_usec) / 1000;
}

static bool _timer_arrived(enum file_sink_type_t type, int time_ms)
{
    if(start[type].tv_sec == 0 && start[type].tv_usec == 0){
        _gettime(&start[type]);
    }
    _gettime(&now[type]);
    if(_tv_diff(&now[type], &start[type]) > time_ms){
        return true;
    }
        
    return false;
}

static void _timer_init(enum file_sink_type_t type)
{
    memset(&start[type], 0, sizeof(start[type]));
    memset(&now[type], 0, sizeof(now[type]));
}


char *filepath_gen(enum file_sink_type_t type,char *buf, size_t len)
{
    char *stype = "fft";
    if(type == FILE_SINK_TYPE_NIQ)
        stype = "niq";
    else if(type == FILE_SINK_TYPE_BIQ)
        stype = "biq";
    else 
        stype = "raw";
    srand(time(NULL));
    snprintf(buf, len, "/tmp/%s_%8.8x", stype, rand() & 0xFFFFFFFF);
    return buf;
}

int file_sink_init(char *filename, enum file_sink_type_t type, int args)
{
    char filegen[256] = {0};

    if(args == 0)
        return -1;
    
    if(!filename)
        filename = filepath_gen(type, filegen, sizeof(filegen));
    printf("generate file:%s\n", filename);
    FILE * _fp = NULL;
    _fp = fopen (filename, "ab");
    if(!_fp){
        return -1;
    }

    file_sink_time_ms[type] = args;
    printf("file sink time ms[%d]:%d\n", type, file_sink_time_ms[type]);
    _timer_init(type);
    fp[type]  = _fp;
    return 0;
}

int file_sink_close(enum file_sink_type_t type)
{
    if(fp[type]){
        fclose(fp[type]);
        fp[type] = NULL;
        return 0;
    }
    return -1;
}


size_t file_sink_work(enum file_sink_type_t type, void *data, size_t len)
{
    if(fp[type] == NULL)
        return 0;

    if(_timer_arrived(type, file_sink_time_ms[type])){
        printf("File Sink time arrived:%d ms\n", file_sink_time_ms[type]);
        file_sink_close(type);
        _timer_init(type);
        return 0;
    }
    return fwrite(data, sizeof(int8_t), len, fp[type]);
}
