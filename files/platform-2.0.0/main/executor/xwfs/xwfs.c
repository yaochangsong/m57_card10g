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

int32_t xwfs_get_disk_info(void *arg){
    int32_t ret = 0;
    printf_debug("xwfs_get_disk_info\n");
    if (xwfs_fd < 0) {
        return -1;
    }
    ret = ioctl(xwfs_fd,IOCTL_XWFS_DISK_GET_INFO,arg);
    return ret;
}


int32_t xwfs_get_file_info(void *arg){
    int32_t ret = 0;
    printf_debug("xwfs_get_disk_info\n");
    if (xwfs_fd < 0) {
        return -1;
    }
    ret = ioctl(xwfs_fd,IOCTL_XWFS_DISK_READ_FILE_INFO,arg);
    return ret;
}


int32_t xwfs_disk_format(void *arg){
    int32_t ret = 0;
    printf_debug("xwfs_disk_format\n");
    if (xwfs_fd < 0) {
        return -1;
    }
    ret = ioctl(xwfs_fd,IOCTL_XWFS_DISK_FORMAT,arg);
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

int32_t xwfs_start_backtrace(void *arg){
    int32_t ret = 0;
    if (xwfs_fd < 0) {
        return -1;
    }
    SW_TO_BACKTRACE_MODE();
    ret = ioctl(xwfs_fd,IOCTL_XWFS_DISK_START_BACKTRACE_FILE_INFO,arg);
    return ret;
}

int32_t xwfs_stop_backtrace(void *arg){
    int32_t ret = 0;
    if (xwfs_fd < 0) {
        return -1;
    }
    SW_TO_AD_MODE();
    ret = ioctl(xwfs_fd,IOCTL_XWFS_DISK_STOP_BACKTRACE_FILE_INFO,arg);
    return ret;
}

int32_t xwfs_refresh_disk_file_buffer(void *arg){
    int32_t ret = 0;
    if (xwfs_fd < 0) {
        return -1;
    }
    ret = ioctl(xwfs_fd,IOCTL_XWFS_DISK_REFRESH_FILE_BUFFER,arg);
    return ret;
}


int xwfs_get_fd(void)
{
    return xwfs_fd;
}


void xwfs_init(void)
{
    printf_note("xwfs init!\n");
#if defined(CONFIG_FS_XW)
    int ret = 0, status = 1;
    STORAGE_STATUS_RSP_ST *psi;
    if (xwfs_fd > 0) {
        return;
    }
    xwfs_fd = open(XWFS_DISK_DEVICE, O_RDWR);
    if(xwfs_fd < 0) {
        perror("open");
        goto exit;
    }
    ret = xwfs_get_disk_info(psi);
    if(ret != 0){
        printf_note("Disk Check Faild!!\n");
        status = 1;
    }else{
        printf_note("Disk check OK!! num=%d, speed=%uKB/s, capacity_bytes=%"PRIu64", used_bytes=%"PRIu64"\n",
                    psi->disk_num, psi->read_write_speed_kbytesps, 
                    psi->disk_capacity[0].disk_capacity_byte, psi->disk_capacity[0].disk_used_byte);

        status = 0;
    }
exit:
    config_save_cache(EX_STATUS_CMD, EX_DISK_STATUS, -1, &status);
#endif
}


