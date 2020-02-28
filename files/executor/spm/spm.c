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
#include "config.h"

struct spm_context *spm_ctx = NULL;

void spm_deal(void *args)
{
    struct spm_context *pctx = spm_ctx;
    if(pctx == NULL){
        printf_err("spm is not init!!\n");
        return;
    }
    pctx->ops->read_iq_data
}


int spm_init(void)
{
#if defined(SUPPORT_SPECTRUM_CHIP)
    spm_ctx = spm_create_chip_context();
#elif defined (SUPPORT_SPECTRUM_FPGA)
    spm_ctx = spm_create_fpga_context();
#endif
    spm_ctx->ops->create();
    return 0;
}


