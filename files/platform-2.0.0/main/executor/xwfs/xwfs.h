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
#ifndef __XWFS_H__
#define __XWFS_H__

#define XWFS_DISK_DEVICE "/dev/xfs_disk"

typedef enum _XWFS_DISK_IOCTL_CMD {
    XWFS_DISK_REFRESH_FILE_BUFFER=0x0,
    XWFS_DISK_READ_FILE_INFO=1,
    XWFS_DISK_FIND_FILE=0x2,
    XWFS_DISK_START_SAVE_FILE=0x3,
    XWFS_DISK_STOP_SAVE_FILE = 0x4,
    XWFS_DISK_DELETE_FILE=0x5,
    XWFS_DISK_START_BACKTRACE_FILE=0x6,
    XWFS_DISK_STOP_BACKTRACE_FILE=0x7,
    XWFS_DISK_GET_INFO=0x8,
    XWFS_DISK_FORMAT=0x9,
}XWFS_DISK_IOCTL_CMD;


#define DISK_IOC_MAGIC  'd'
#define IOCTL_XWFS_DISK_REFRESH_FILE_BUFFER          _IOW(DISK_IOC_MAGIC, XWFS_DISK_REFRESH_FILE_BUFFER, uint32_t)
#define IOCTL_XWFS_DISK_READ_FILE_INFO               _IOW(DISK_IOC_MAGIC, XWFS_DISK_READ_FILE_INFO, uint32_t)
#define IOCTL_XWFS_DISK_FIND_FILE_INFO               _IOW(DISK_IOC_MAGIC, XWFS_DISK_FIND_FILE, uint32_t)
#define IOCTL_XWFS_DISK_START_SAVE_FILE_INFO         _IOW(DISK_IOC_MAGIC, XWFS_DISK_START_SAVE_FILE, uint32_t)
#define IOCTL_XWFS_DISK_STOP_SAVE_FILE_INFO          _IOW(DISK_IOC_MAGIC, XWFS_DISK_STOP_SAVE_FILE, uint32_t)
#define IOCTL_XWFS_DISK_DELETE_FILE_INFO             _IOW(DISK_IOC_MAGIC, XWFS_DISK_DELETE_FILE, uint32_t)
#define IOCTL_XWFS_DISK_START_BACKTRACE_FILE_INFO    _IOW(DISK_IOC_MAGIC, XWFS_DISK_START_BACKTRACE_FILE, uint32_t)
#define IOCTL_XWFS_DISK_STOP_BACKTRACE_FILE_INFO     _IOW(DISK_IOC_MAGIC, XWFS_DISK_STOP_BACKTRACE_FILE, uint32_t)
#define IOCTL_XWFS_DISK_GET_INFO                     _IOW(DISK_IOC_MAGIC, XWFS_DISK_GET_INFO, uint32_t)
#define IOCTL_XWFS_DISK_FORMAT                       _IOW(DISK_IOC_MAGIC, XWFS_DISK_FORMAT, uint32_t)

extern void xwfs_init(void);

#endif
