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
#ifndef _FILE_SINK_H
#define _FILE_SINK_H

enum file_sink_type_t{
    FILE_SINK_TYPE_FFT,
    FILE_SINK_TYPE_NIQ,
    FILE_SINK_TYPE_BIQ,
    FILE_SINK_TYPE_RAW,
    FILE_SINK_TYPE_MAX,
};

extern int file_sink_init(char *filename, enum file_sink_type_t type, int args);
extern size_t file_sink_work(enum file_sink_type_t type, void *data, size_t len);
#endif

