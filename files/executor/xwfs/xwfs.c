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
*  Rev 1.0   02 Jan 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"

static int xwfs_fd = -1;

int32_t xwfs_start_save_file(void *arg){
    int32_t ret = 0;
    printf_note("xwfs_start_save_file\n");
    if (xwfs_fd < 0) {
        return -1;
    }
    ret = ioctl(xwfs_fd,IOCTL_XWFS_DISK_START_SAVE_FILE_INFO,arg);
    return ret;
}

int32_t xwfs_stop_save_file(void *arg){
    int32_t ret = 0;
    printf_note("xwfs_stop_save_file\n");
    if (xwfs_fd < 0) {
        return -1;
    }
    ret = ioctl(xwfs_fd,IOCTL_XWFS_DISK_STOP_SAVE_FILE_INFO,arg);
    return ret;
}

int32_t xwfs_get_file_size_by_name(char *name, void *value, uint32_t value_len){
    int32_t ret = 0;
    if (xwfs_fd < 0) {
        return -1;
    }
    if(strlen(name) < value_len){
        printf_warn("name length[%d] is shorter than value length[%d]\n", strlen(name), value_len);
        return -1;
    }
    ret = ioctl(xwfs_fd,IOCTL_XWFS_DISK_FIND_FILE_INFO,name);
    memcpy(value, name, value_len);
    return ret;
}


int32_t xwfs_delete_file(void *arg){
    int32_t ret = 0;
    if (xwfs_fd < 0) {
        return -1;
    }
    ret = ioctl(xwfs_fd,IOCTL_XWFS_DISK_DELETE_FILE_INFO,arg);
    return ret;
}


void xwfs_init(void)
{
    printf_note("xwfs init!\n");
#if defined(SUPPORT_XWFS)
    if (xwfs_fd > 0) {
        return;
    }
    xwfs_fd = open(XWFS_DISK_DEVICE, O_RDWR);
    if(xwfs_fd < 0) {
        perror("open");
        return;
    }
#endif
}

