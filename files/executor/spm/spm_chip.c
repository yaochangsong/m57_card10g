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
*  Rev 1.0   21 Feb. 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#include <math.h>
#include "config.h"
#include "spm.h"

typedef int16_t fft_data_type;

/* Allocate zeroed out memory */
static inline void *zalloc(size_t size)
{
	return calloc(1, size);
}

static int spm_chip_create(void)
{

}

iq_t *_iqdata_once  = NULL;

static ssize_t spm_chip_read_iq_data(void **data)
{
    ssize_t r = 0;
    iq_t *iqdata = NULL;
    
    //iqdata = specturm_rx0_read_data(&r);
    *data = iqdata;
    return r;
}

/* need to free */
static ssize_t spm_chip_read_fft_data(void **data, uint32_t fft_size)
{
    float *floatdata = NULL;
    fft_t *fftdata = NULL;
    iq_t *iqdata = NULL;
    ssize_t r = 0;
    //iqdata = specturm_rx0_read_data(&r);
    
    floatdata = zalloc(fft_size*4);
    if(floatdata == NULL)
        goto err;
    //fft_spectrum_iq_to_fft_handle(iqdata, fft_size, fft_size*2, floatdata);
    
    fftdata = zalloc(fft_size*4);
    if(fftdata == NULL){
        free(floatdata);
        floatdata = NULL;
        goto err;
    }
    FLOAT_TO_SHORT(floatdata, fftdata, fft_size);
    free(floatdata);
    floatdata = NULL;
    *data = fftdata;
    
    return fft_size;
err:
    *data = NULL;
    return -1;
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

/* 频谱数据整理 */
static fft_data_type *spm_chip_data_order(fft_data_type *fft_data, 
                                size_t fft_len,  
                                size_t *order_fft_len,
                                void *arg)
{
    struct spm_run_parm *hparam;
    float sigle_side_rate, side_rate;
    uint32_t single_sideband_size;

    if(fft_data == NULL || fft_len == 0)
        return NULL;
    hparam = (struct spm_run_parm *)arg;
    /* 1：去边带 */
    /* 获取边带率 */
    side_rate  = get_side_band_rate(hparam->bandwidth);
    sigle_side_rate = (1.0-1.0/side_rate)/2.0;
    printf_note("side_rate=%f, sigle_side_rate = %f\n", side_rate, sigle_side_rate);
    
    single_sideband_size = fft_len*sigle_side_rate;
    printf_note("single_sideband_size = %u\n", single_sideband_size);
    *order_fft_len = fft_len - 2 * single_sideband_size;
    return  fft_data + single_sideband_size;
}


static int spm_chip_send_fft_data(void *data, size_t fft_len, void *arg)
{
    #define HEAD_BUFFER_LEN  512 
    uint8_t *ptr_header = NULL;
    uint32_t header_len = 0;
    size_t data_byte_size = 0;

    if(data == NULL || fft_len == 0 || arg == NULL)
        return -1;
    data_byte_size = fft_len * sizeof(fft_t);
#if (defined SUPPORT_DATA_PROTOCAL_AKT)
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
    hparam->data_len = data_byte_size; 
    hparam->type = SPECTRUM_DATUM_FLOAT;
    hparam->ex_type = SPECTRUM_DATUM;
    if((ptr_header = akt_assamble_data_frame_header_data(&header_len, arg)) == NULL){
        return -1;
    }
#elif defined(SUPPORT_DATA_PROTOCAL_XW)
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
    hparam->data_len = data_byte_size; 
    hparam->type = DEFH_DTYPE_FLOAT;
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

    udp_send_vec_data(iov, 2);
    safe_free(ptr_header);
    return (header_len + data_byte_size);
}

/*********************************************************
    功能:组装IQ数据协议头
    输入参数：
        @data_len: IQ数据长度
        @arg: 数据属性指针
    输出参数:
        @hlen: (IQ)数据头长度
    返回:
        NULL: 失败
        非空: 协议头指针; 使用后需要释放
*********************************************************/
static void *_assamble_iq_header(size_t subch, size_t *hlen, size_t data_len, void *arg)
{
    uint8_t *ptr = NULL, *ptr_header = NULL;
    uint32_t header_len = 0;
    
    if(data_len == 0 || arg == NULL)
        return NULL;
    
#ifdef SUPPORT_DATA_PROTOCAL_AKT
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
    hparam->data_len = data_len;
    hparam->type = BASEBAND_DATUM_IQ;
    hparam->ex_type = DEMODULATE_DATUM;
    //hparam->sub_ch = subch;
    if((ptr_header = akt_assamble_data_frame_header_data(&header_len, arg))== NULL){
        return NULL;
    }
    
#elif defined(SUPPORT_DATA_PROTOCAL_XW)
    struct spm_run_parm *hparam;
    hparam = (struct spm_run_parm *)arg;
    hparam->data_len = data_len; 
    hparam->type = DEFH_DTYPE_BB_IQ;
    hparam->ex_type = DFH_EX_TYPE_DEMO;
   // hparam->sub_ch = subch;
    ptr_header = xw_assamble_frame_data(&header_len, arg);
    if(ptr_header == NULL)
        return NULL;
#endif
    *hlen = header_len;

    return ptr_header;

}

#define DEFAULT_IQ_SEND_BYTE 512
static size_t iq_send_unit_byte = DEFAULT_IQ_SEND_BYTE;    /* IQ发送长度 */
static int spm_chip_send_iq_data(void *data, size_t len, void *arg)
{
    uint32_t header_len = 0;
    size_t _send_byte = (iq_send_unit_byte > 0 ? iq_send_unit_byte : DEFAULT_IQ_SEND_BYTE);
    uint8_t *hptr = NULL;
    
    if(data == NULL || len == 0 || arg == NULL)
        return -1;

    if(len < _send_byte)
        return -1;
    if((hptr = _assamble_iq_header(1, &header_len, _send_byte, arg)) == NULL){
        printf_err("assamble head error\n");
        return -1;
    }
   
    int i, index,sbyte;
    uint8_t *pdata;
    struct iovec iov[2];
    iov[0].iov_base = hptr;
    iov[0].iov_len = header_len;
    index = len / _send_byte;
    sbyte = index * _send_byte;
    pdata = (uint8_t *)data;
    for(i = 0; i<index; i++){
        iov[1].iov_base = pdata;
        iov[1].iov_len = _send_byte;
        udp_send_vec_data(iov, 2);
        pdata += _send_byte;
    }
   
    safe_free(hptr);
    
    return (header_len + len);
}


static int spm_chip_send_cmd(void *cmd, void *data, size_t data_len)
{
#ifdef SUPPORT_PROTOCAL_AKT
  //  poal_send_active_to_all_client((uint8_t *)data, data_len);
#endif
}

static int spm_chip_convet_iq_to_fft(void *iq, void *fft, size_t fft_len)
{
    if(iq == NULL || fft == NULL || fft_len == 0)
        return -1;
    //fft_spectrum_iq_to_fft_handle((short *)iq, fft_len, fft_len*2, (float *)fft);
    return 0;
}

static int spm_chip_save_data(void *data, size_t len)
{
    return 0;
}

static int spm_chip_backtrace_data(void *data, size_t len)
{
    return 0;
}

static int spm_chip_set_psd_analysis_enable(bool enable)
{
    return 0;
}


static int spm_chip_get_psd_analysis_result(void *data, size_t len)
{
    return 0;
}


static int spm_chip_close(void)
{
    return 0;
}


static const struct spm_backend_ops spm_ops = {
    .create = spm_chip_create,
    .read_iq_data = spm_chip_read_iq_data,
    .data_order = spm_chip_data_order,
    .send_fft_data = spm_chip_send_fft_data,
    .send_iq_data = spm_chip_send_iq_data,
    .send_cmd = spm_chip_send_cmd,
    .convet_iq_to_fft = spm_chip_convet_iq_to_fft,
    .set_psd_analysis_enable = spm_chip_set_psd_analysis_enable,
    .get_psd_analysis_result = spm_chip_get_psd_analysis_result,
    .save_data = spm_chip_save_data,
    .backtrace_data = spm_chip_backtrace_data,
    .close = spm_chip_close,
};

struct spm_context * spm_create_chip_context(void)
{
    int ret = -ENOMEM;
    unsigned int len;
    struct spm_context *ctx = zalloc(sizeof(*ctx));
    if (!ctx)
        goto err_set_errno;
    
    ctx->ops = &spm_ops;
    ctx->pdata = &config_get_config()->oal_config;
    
    
err_set_errno:
    errno = -ret;
    return ctx;

}
