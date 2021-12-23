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
*  Rev 1.0   04 Jan 2021   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _M57_H
#define _M57_H

/* 该协议由57所提供: 协议数据和命令走一个通道，需要区别 */


/* 协议头定义 */
struct m57_header_st {
    uint16_t header;        /* 头标志0x5751*/
    uint8_t  prio : 4;      /* 优先级：0:最低,1:普通,2:优先,3:最高 */
    uint8_t  type : 4;      /* 包类型: 数据：0,控制：2 */
    uint8_t  number;        /* 流水号 */
    uint16_t len;           /* 包长度：所有字段总长度 */
    uint16_t resv;          /* 保留   */
};

/* 命令头定义 */
struct m57_ex_header_cmd_st {
    uint16_t cmd;                   /* 命令类型*/
    uint16_t len;                   /* 总长,包括命令头和命令数据 */
    uint16_t resv;                  /* 保留 */
    uint16_t resv1;                 /* 保留 */
};


/* 数据头定义 */
struct m57_data_st {
    uint8_t  *payload;              /* 内容   */
};

/* 加载命令数定义 */
struct load_info{
    uint16_t chipid;
    uint8_t is_reload;
    uint8_t resv;
    uint8_t encry;
    uint8_t resv2;
    uint32_t file_len;
}__attribute__ ((packed));

/* 数据协议定义 */
struct data_frame_t{
    uint16_t py_dist_addr;
    uint16_t py_src_addr;
    uint16_t dist_addr;
    uint16_t len;
    uint16_t type;
    uint16_t src_addr;
    uint16_t port;
}__attribute__ ((packed));


/* 异步加载文件定义 */
struct asyn_load_ctx_t{
    char *filename;     //加载文件名
    FILE *file;         //文件句柄
    void *buffer;       //加载缓冲区
    void *ops;         //操作制作
    void *client;       //加载客户端
    pthread_t tid;      //线程id
    struct stat *fstat; //文件属性
    char *thread_name;
    bool result;        //加载结果
    uint64_t total_load;
    int reminder;
    #define LOAD_BITFILE_TIMEOUT (60)
    struct uloop_timeout timeout;
};

#define M57_SYNC_HEADER 0x5751
//#define CARD_SLOT_NUM(x) (((x)>>8) & 0xff)    //05
//#define CARD_CHIP_NUM(x) ((x)& 0xff)    //02

typedef enum {
    M57_DATA_TYPE =  0,
    M57_CMD_TYPE  =  2,
}m57_payload_type;

typedef enum {
    M57_PRIO_LOW        = 0,
    M57_PRIO_NORMAL     = 1,
    M57_PRIO_HIGH       = 2,
    M57_PRIO_URGENT     = 3,
}m57_prio_type;


typedef enum {
    C_RET_CODE_SUCCSESS         = 0,
    C_RET_CODE_FORMAT_ERR       = 1,
    C_RET_CODE_PARAMTER_ERR     = 2,
    C_RET_CODE_PARAMTER_INVALID = 3,
    C_RET_CODE_CHANNEL_ERR      = 4,
    C_RET_CODE_INVALID_MODULE   = 5,
    C_RET_CODE_INTERNAL_ERR     = 6,
    C_RET_CODE_PARAMTER_NOT_SET = 7,
    C_RET_CODE_PARAMTER_TOO_LONG= 8,
    C_RET_CODE_LOCAL_CTRL_MODE  = 9,
    C_RET_CODE_DEVICE_BUSY      = 10,
    C_RET_CODE_FILE_NOT_FOUND   = 11,
    C_RET_CODE_FILE_EXISTS      = 12,
    C_RET_CODE_HEADER_ERR       = 13,
} card_return_code;

/* 命令类型 */
typedef enum {
    /*1 心跳包*/
    CCT_BEAT_HART   = 0x0000,
    /* 2 通道注册包 */
    CCT_REGISTER    = 0x0001,
    /* 3 通道注销包 */
    CCT_UNREGISTER  = 0x0002,
    /* 4 数据订阅包 */
    CCT_DATA_SUB    = 0x0003,
    /*  5 通道退订包 */
    CCT_DATA_UNSUB  = 0x0004,
    /* 6 加载命令包 */
    CCT_LOAD        = 0x0005,
    /* 7 加载命令回复包 */
    CCT_RSP_LOAD    = 0x0006,
    /* 8 卸载命令包 */
    CCT_UNLOAD      = 0x0007,
    /* 卸载命令回复包 */
    CCT_RSP_UNLOAD  = 0x0008,
    /* 文件传输命令包 */
    CCT_FILE_TRANSFER = 0x0009,
    /* 读取板卡信息命令包 */
    CCT_READ_BOARD_INFO = 0x0012,
    /* 读取板卡信息命令回复包 */
    CCT_RSP_READ_BOARD_INFO = 0x0013,
    /* 写入板卡信息命令包 */
    CCT_WRITE_BOARD_INFO = 0x0014,
    /* 写入板卡信息命令回复包 */
    CCT_RSP_WRITE_BOARD_INFO = 0x0015,
    /* 平台信息查询命令包 */
    CCT_LIST_INFO            = 0x0016,
    /* 平台信息查询命令回复包 */
    CCT_RSP_LIST_INFO        = 0x0017,
    /* 主控节点注册命令包 */
    CCT_MASTER_NODE_REGISTER = 0x0018,
    /* 密钥读取E2PROM命令,自定义 */
    CCT_KEY_READ_E2PROM = 0x0019,
    /* 密钥读取E2PROM命令,自定义 */
    CCT_KEY_WRITE_E2PROM = 0x001a,
}card_cmd_type;

#define M57_CARD_STATUS_LOAD_FAILD   -5
#define M57_CARD_STATUS_LINK_FAILD   -7
#define M57_CARD_STATUS_OK            0

extern bool m57_parse_header(void *client, char *buf, int len, int *head_len, int *code);
extern bool m57_execute(void *client, int *code);
extern void m57_send_error(void *client, int code, void *args);
extern void m57_send_resp(void *client, int code, void *args);
extern void m57_send_heatbeat(void *client);
extern int m57_unload_bitfile_from_fpga(uint16_t chip_id);
extern int m57_unload_bitfile_by_client(struct sockaddr_in *addr);
extern int m57_unload_bitfile_all(void);
extern int m57_loading_bitfile_to_fpga_asyn(void *client, char *filename);
extern int m57_stop_load(void *client);

#endif

