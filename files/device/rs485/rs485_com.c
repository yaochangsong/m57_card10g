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
*  Rev 1.0   04 June 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"
#include "rs485_com.h"

static struct _frame_data{
    uint8_t     header;
    uint16_t    res;
    uint8_t     type;
    uint16_t    valid_len;
    uint8_t     data[0];
    uint8_t     end;
}__attribute__ ((packed));

static struct _frame_read_x{
    uint8_t *frame_buffer;
    size_t offset;
};

static struct _frame_read_x frame_read;

static ssize_t _rs485_com_read_frame(uint8_t *rev_data, size_t rev_nread, struct _frame_data **oframe)
{
    struct _frame_read_x *fread = &frame_read;
    uint8_t *databuf = fread->frame_buffer;
    
    ssize_t ret = 0;
    size_t nread = 0, i;
    uint16_t valid_data_len = 0;

    nread = rev_nread;
    printf_note("nread:%d:\n", nread);
    if(nread > FRAME_MAX_LEN)
        nread = FRAME_MAX_LEN;
    memcpy(databuf+fread->offset, rev_data, nread);
    fread->offset += nread;
    printf_note("fread->offset:%d:\n", fread->offset, databuf[0]);
    if(databuf[0] == FRAME_HEADER){
        if(fread->offset >= FRAME_DATA_LEN_OFFSET+1){
            for(i = 0; i< fread->offset; i++){
                printfn(" 0x%x",databuf[i]);
            }
            printfn("\n");
            
            valid_data_len = ntohs(*(uint16_t *)(databuf+FRAME_DATA_LEN_OFFSET));
            printf_note("valid_data_len = 0x%x\n", valid_data_len);
            if(valid_data_len >sizeof(int32_t)){
                printf_warn("data len is too long\n", valid_data_len);
                return -1;   /* error */
            }
        }
        if((fread->offset >= FRAME_DATA_LEN_OFFSET+FRAME_DATA_LEN+valid_data_len + 1) && /* read end */
            (databuf[fread->offset -1] == FRAME_END)){ /* endflag is ok */
            struct _frame_data *frame = NULL;
            printf_debug("receive len:%d:\n", fread->offset);
            for(i = 0; i< fread->offset; i++){
                printfd(" 0x%x",databuf[i]);
            }
            printfd("\n");
            frame = (struct _frame_data *)calloc(1, sizeof(struct _frame_data) + valid_data_len);
            *oframe = frame;
            memcpy(frame, databuf,sizeof(struct _frame_data) + valid_data_len);
            frame->valid_len = ntohs(frame->valid_len);
            printfn("valid_len = %x, type=%x, size=%d\n",frame->valid_len, frame->type, sizeof(struct _frame_data) + valid_data_len);
            ret = fread->offset;
            fread->offset = 0;
        }
    }else{
        printf_warn("Data Frame Header Error[0x%02x]\n", databuf[0]);
        ret = -1;   /* error */
    }
    printf_note("ret = %d\n", ret);
    return ret; /* >0 read over; =0 reading; <0 read error*/
}


static ssize_t _rs485_com_recv(uint8_t *pdata, int32_t total_len, void *data)
{
    ssize_t frame_len = 0;
    struct _frame_data *frame = NULL;
    struct _frame_read_x *fread = &frame_read;
    uint16_t len = 0;
    int32_t idata = 0;

    if(pdata == NULL){
       return -1;
    }
    
    /*frame_len >0 read over; =0 reading; <0 read error*/
    if ((frame_len = _rs485_com_read_frame(pdata, total_len, &frame)) <= 0)
       return frame_len;
    /* read over */
    if(frame == NULL){
        printf_warn("frame is null\n");
        return -1;
    }
#if 0
    char *ptr = (char *)frame;
    int i;
    printf_note("receive frame:valid_len:%x, offset:%d, frame->type=%x\n", frame->valid_len, frame_len, frame->type);
    for(i = 0; i< frame_len; i++){
        printfn(" 0x%x",*ptr++);
    }
    printfn("\n");
#endif
    len = frame->valid_len;
    if(len > sizeof(uint32_t)){
        printf_err("NOT Support Data Type len:[len=%d]\n", len);
        return -1;
    }
    
    memcpy(&idata, &frame->header+FRAME_DATA_LEN_OFFSET+FRAME_DATA_LEN, len);
    if(len == sizeof(int32_t)){
        idata = htonl(*(int32_t *)&idata);
    }else if(len == sizeof(int16_t)){
        idata = htons(*(int16_t *)&idata);
    }else if(len == sizeof(int8_t)){
        idata = *(int8_t *)&idata;
    }else{
        printf_err("NOT Support Data Type len:[len=%d]\n", len);
        return -1;
    }
    switch(frame->type){
        case RS_485_LOW_NOISE_GET_RSP_CMD:
            printf_note("RS_485_LOW_NOISE_GET_RSP_CMD: 0x%x\n", idata);
            break;
        case RS_485_LOW_NOISE_SET_RSP_CMD:
            printf_note("RS_485_LOW_NOISE_SET_RSP_CMD:0x%x\n", idata);
            break;
        default:
            printf_err("not support cmd[%d]\n", frame->type);
    }
    memcpy(data, idata, len);
exit:
    memset(frame_read.frame_buffer, 0, FRAME_MAX_LEN);
    frame_read.offset = 0;
    safe_free(frame);
    return 1;
}

static int8_t _rs485_assemble_send_data(uint8_t **buffer, uint8_t cmd, void *data, size_t len)
{
    struct _frame_data *fdata;
    int n;
    uint8_t *pend;
    int32_t idata = 0;
    
    if(len > FRAME_MAX_LEN - FRAME_DATA_LEN_OFFSET)
        len = FRAME_MAX_LEN - FRAME_DATA_LEN_OFFSET;

    n = sizeof(struct _frame_data) + len;
    fdata = (struct _frame_data *)calloc(1, n);
    fdata->header = FRAME_HEADER;
    fdata->res = 0x0000;
    fdata->type = cmd;
    fdata->valid_len = htons(len);
    if(data && len > 0){
        if(len == sizeof(int32_t)){
            idata = htonl(*(int32_t *)data);
        }else if(len == sizeof(int16_t)){
            idata = htons(*(int16_t *)data);
        }else if(len == sizeof(int8_t)){
            idata = *(int8_t *)data;
        }else{
            printf_err("NOT Support Data Type:[len=%d]\n", len);
            return -1;
        }
        memcpy(fdata->data, &idata, len);
    }
    pend = fdata->data + len;
    *pend = FRAME_END;
    *buffer = fdata;
    return n;
}

int8_t rs485_com_set(int32_t cmd, void *pdata, size_t len)
{
    int8_t ret = 0, n = 0, i = 0;
    char *buffer;
    
    if(cmd >= RS_485_MAX_CMD){
        printf_err("not support cmd: %d\n", cmd);
        return -1;
    }
    n = _rs485_assemble_send_data(&buffer, cmd, pdata, len);
    if(n > 0 && n < FRAME_MAX_LEN){
        printfn("send byte[%d]:", n);
        for(i = 0; i< n; i++){
            printfn("%02x ", buffer[i]);
        }
        printfn("\n");
        rs485_send_data_by_serial(buffer, n);
    }else{
        printf_err("assemble err n: %d\n", n);
        ret = -1;
    }
    safe_free(buffer);
    return ret;
}

/*551 通信协议*/
//Dev:255;ID:1;ChCtrl:1;\r\n
static int8_t _rs485_assemble_send_data_v2(uint8_t **buffer, uint8_t cmd, void *data)
{
    char *fdata;
    int n;
    
    #define DEV_NAME_ 255
    #define ID_ 1

    n = FRAME_MAX_LEN - 1;
    fdata = (char *)calloc(1, n);
    *buffer = fdata;
    if(cmd == RS_485_LOW_NOISE_SET_CMD){
        uint8_t idata;
        idata = *(uint8_t *)data;
        if(idata > 3){
            printf_warn("cmd: %d, value=%d is out of range!\n", cmd, idata);
            return -1;
        }
        sprintf(fdata, "Dev:%d;ID:%d;ChCtrl:%d;\r\n", DEV_NAME_, ID_, idata);
    }
    return strlen(fdata);
}


int8_t rs485_com_set_v2(int32_t cmd, void *pdata)
{
    int8_t ret = 0, n = 0, i = 0;
    char *buffer;
    
    if(cmd >= RS_485_MAX_CMD){
        printf_err("not support cmd: %d\n", cmd);
        return -1;
    }
    n = _rs485_assemble_send_data_v2(&buffer, cmd, pdata);
    if(n > 0 && n < FRAME_MAX_LEN){
        printfi("send byte[%d]:", n);
        printfi("%s\n", buffer);
        for(i = 0; i< n; i++){
            printfi("%02x ", buffer[i]);
        }
        printfi("\n");
        /* NOTE:  
            If it is set to block sending here; there may be a big delay after sending slow
        */
        rs485_send_data_by_serial(buffer, n);
    }else{
        printf_info("assemble err n: %d\n", n);
        ret = -1;
    }
    safe_free(buffer);
    return ret;
}


int8_t rs485_com_get(int32_t cmd, void *data)
{
    ssize_t ret = 0;
    int nbyte = 0, i;
    uint8_t buffer[FRAME_MAX_LEN];
    #define TIME_OUT_MS 2000
    
    rs485_com_set(cmd, NULL, 0);
    do{
        nbyte = rs485_read_block_timeout(buffer, TIME_OUT_MS);
        if(nbyte <= 0){
            if(nbyte == 0){
                printf_err("read timeout\n");
            }else{
                printf_err("read error[%d]\n", nbyte);
            }
            return -1;
        }
        printfn("recv[%d]:\n", nbyte);
        for(i = 0; i < nbyte; i++)
            printfn("%02x ", buffer[i]);
        printfn("\n");

        /*ret >0 read over; =0 reading; <0 read error*/
        ret = _rs485_com_recv(buffer, nbyte, data);
    }while(ret == 0);
    if(ret < 0){
        printf_err("Read Error!\n");
    }else{
        printf_note("read over![data=%d]\n", *(uint8_t *)data);
    }

    return ret;
}


int8_t rs485_com_init(void)
{
    frame_read.frame_buffer = calloc(1, FRAME_MAX_LEN);
    frame_read.offset = 0;
    return 0;
}


