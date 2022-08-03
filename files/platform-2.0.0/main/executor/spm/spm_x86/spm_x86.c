/******************************************************************************
*  Copyright 2019, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   28 July. 2021   yaochangsong
*  Initial revision.
******************************************************************************/
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
#include "spm_x86.h"
#include "../../../protocol/resetful/data_frame.h"
#include "../../../bsp/io.h"
#include "../agc/agc.h"

static inline void *zalloc(size_t size)
{
    return calloc(1, size);
}

FILE *_file_fd[4];
static  void init_write_file(char *filename, int index)
{
    _file_fd[index] = fopen(filename, "w+b");
    if(!_file_fd[index]){
        printf_err("Open file error!\n");
    }
}

static inline void close_file(void)
{
    for(int i = 0; i < 4; i++){
        if(_file_fd[i] > 0){
            fclose(_file_fd[i]);
        }
    }
    
}


static inline int write_file_ascii(int16_t *pdata, int n, int index)
{
    char _file_buffer[32] = {0};
    for(int i = 0; i < n; i++){
        sprintf(_file_buffer, "%d ", *pdata ++);
        fwrite((void *)_file_buffer,strlen(_file_buffer), 1, _file_fd[index]);
    }


    return 0;
}

static inline int write_float_file_ascii(float *fdata, int n, int index)
{
    char _file_buffer[64] = {0};
    for(int i = 0; i < n; i++){
        sprintf(_file_buffer, "%f ", *fdata ++);
        fwrite((void *)_file_buffer,strlen(_file_buffer), 1, _file_fd[index]);
    }


    return 0;
}


static inline int write_file_nfft(int16_t *pdata, int n, int index)
{
    fwrite((void *)pdata, sizeof(int16_t), n, _file_fd[index]);

    return 0;
}

static void printf_fft(fft_t *ptr, size_t len)
{
    for(int i = 0; i < len; i++){
        printf("%x ", *ptr++);
    }
    printf("\n");
}

static void *_creat_sin_data(uint32_t points, uint32_t maxval, void *data)
{
    //data = k*sin(w*t + k)
    #define PI (3.1415926)
    int16_t *iptr = NULL, *header;
    //float f0 = 100000;
    float f0 = 1000;
    float fs = MHZ(10);
    static float ph = PI;

    if(data == NULL)
        return NULL;
    
    iptr = header = data;

    double t,y;
    int x;
    for(x = 0; x < points; x++){
        y = 1000*cos(x*2.0*PI*f0/fs+ph);
        *iptr++ = (int16_t)y;
    }
     ph+=PI;
    if(ph > 1000)
        ph = 0;
    return header;
}


static int spm_x86_create(void)
{
    printf_note("X86 Spm\n");
    return 0;
}

static ssize_t spm_x86_read_fft_data(int ch,  void **data, void *args)
{
    float *floatdata = NULL;
    static fft_t *fftdata[MAX_RADIO_CHANNEL_NUM] = {NULL};
    static size_t fft_size_dup[MAX_RADIO_CHANNEL_NUM] = {0};
    ssize_t r = 0;
    size_t fft_size;
    struct spm_run_parm *run_args;
    iq_t *iqdata = NULL;
    
    run_args = (struct spm_run_parm *)args;
    if(run_args == NULL || ch >= MAX_RADIO_CHANNEL_NUM)
        goto err;
    
    fft_size = run_args->fft_size;
    if(fft_size == 0)
        goto err;
    
    iqdata = zalloc(fft_size*4);
    _creat_sin_data(fft_size*2, 0, iqdata);

    floatdata = zalloc(fft_size*4);
    if(floatdata == NULL)
        goto err;
    fft_spectrum_iq_to_fft_handle((short *)iqdata, fft_size, fft_size*2, floatdata);
    if(fft_size_dup[ch] < fft_size){
        fft_size_dup[ch] = fft_size;
        fftdata[ch] = realloc(fftdata[ch], fft_size_dup[ch]*4);
        if(fftdata[ch] == NULL){
            free(floatdata);
            floatdata = NULL;
            goto err;
        }
    }
    memset(fftdata[ch], 0, fft_size_dup[ch]*4);
    FLOAT_TO_SHORT(floatdata, fftdata[ch], fft_size);
    free(floatdata);
    floatdata = NULL;

    *data = fftdata[ch];
    //*data = iqdata;

    return (fft_size*sizeof(fft_t));
err:
    safe_free(iqdata);
    *data = NULL;
    return -1;
}

static int spm_x86_send_fft_data(void *data, size_t fft_len, void *arg)
{
    #define HEAD_BUFFER_LEN  512 
    uint8_t *ptr_header = NULL;
    uint32_t header_len = 0;
    size_t data_byte_size = 0;
	struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;

    if(data == NULL || fft_len == 0 || arg == NULL)
        return -1;

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
    safe_free(ptr_header);
    usleep(100);
    return (header_len + data_byte_size);
}

static int spm_x86_stream_start(int ch, int subch, uint32_t len,uint8_t continuous, enum stream_type type)
{
    return 0;
}

static int spm_x86_stream_stop(int ch, int subch, enum stream_type type)
{
    return 0;
}

static int spm_x86_close(void *_ctx)
{
    return 0;
}

static int spm_x86_scan(uint64_t *s_freq_offset, uint64_t *e_freq, uint32_t *scan_bw, uint32_t *bw, uint64_t *m_freq)
{
    //#define MAX_SCAN_FREQ_HZ (6000000000)
    uint64_t _m_freq;
    uint64_t _s_freq, _e_freq;
    uint32_t _scan_bw, _bw;
    
    _s_freq = *s_freq_offset;
    _e_freq = *e_freq;
    _scan_bw = *scan_bw;
    if((_e_freq - _s_freq)/_scan_bw > 0){
        _bw = _scan_bw;
        *s_freq_offset = _s_freq + _scan_bw;
    }else{
        _bw = _e_freq - _s_freq;
        *s_freq_offset = _e_freq;
    }
    *scan_bw = _scan_bw;
    _m_freq = _s_freq + _scan_bw/2;
#ifdef CONFIG_BSP_SSA_MONITOR
    if (_m_freq >= _e_freq) {
        _m_freq = _s_freq + _bw/2;
    }
#endif
    //fix bug:中频超6G无信号 wzq
    #if 0
    if (_m_freq > MAX_SCAN_FREQ_HZ){
        _m_freq = MAX_SCAN_FREQ_HZ;
    }
    #endif
    *bw = _bw;
    *m_freq = _m_freq;

    return 0;
}


static const struct spm_backend_ops spm_ops = {
    .create = spm_x86_create,
    .read_fft_data = spm_x86_read_fft_data,
    .send_fft_data = spm_x86_send_fft_data,
    .stream_start = spm_x86_stream_start,
    .stream_stop = spm_x86_stream_stop,
    .spm_scan = spm_x86_scan,
    .close = spm_x86_close,
};

struct spm_context * spm_create_context(void)
{
    int ret = -ENOMEM;
    unsigned int len;
    struct spm_context *ctx = zalloc(sizeof(*ctx));
    if (!ctx)
        goto err_set_errno;
    
    ctx->ops = &spm_ops;
    ctx->pdata = &config_get_config()->oal_config;
    //init_write_file("tmp/fft", 0);

err_set_errno:
    errno = -ret;
    return ctx;
}


