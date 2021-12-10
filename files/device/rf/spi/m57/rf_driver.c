/*
* Copyright (C) 2013 - 2016  Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person
* obtaining a copy of this software and associated documentation
* files (the "Software"), to deal in the Software without restriction,
* including without limitation the rights to use, copy, modify, merge,
* publish, distribute, sublicense, and/or sell copies of the Software,
* and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or (b) that interact
* with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in this
* Software without prior written authorization from Xilinx.
*
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <math.h>
#include <linux/spi/spidev.h>
#include <stdbool.h>
#include "rf_driver.h"
#include "config.h"
#include "../../../../log/log.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

struct rf_info_t rf_info[MAX_XDMA_RF_CARD_NUM];
static struct module_info_t module_info[] = {
        {0, MODE_CODE_CDB,  "CDB Module"},
        {1, MODE_CODE_LF,   "LF Module"},
        {2, MODE_CODE_IF,   "IF Module"},
        {3, MODE_CODE_CLK,  "Clk Module"},
        {4, MODE_CODE_RF,   "RF Module"},
        {5, MODE_CODE_FULL, "Full fabric"},
};


static const char *const vendor_code_str[] = {
    [0] = "57#",
    [1] = "chongqing huiling",
    [2]  = "Electrinic University",
    [3] = "chengdu MircoBand",
    [4] = "chengdu Flex"    , 
    [5] = "chengdu xinren", 
    [6] = "chengdu zhengxin", 
    [7] = "chengdu zhongya", 
    [8] = "Telecom 5#", 
    [9] = "chengdu xinghang", 
    [10] = "Tongfang Electronic",
};

static void _print_array(uint8_t *ptr, ssize_t len)
{
    if(ptr == NULL || len <= 0)
        return;
    
    for(int i = 0; i< len; i++){
        printfn("%02x ", *ptr++);
    }
    printfn("\n");
}

static uint8_t _spi_checksum(uint8_t *buffer,uint8_t len){
    uint8_t check_sum = 0;
    uint8_t i, *ptr = buffer;
    
    check_sum = *ptr++;
    for(i = 1; i < 4+len; i++){
        check_sum ^= *ptr++;
    }
    return check_sum;
}

static bool _spi_data_check(uint8_t *buffer, int len)
{
    return true;
}

static int _spi_send_data_by_reg(uint8_t *send_buffer, size_t send_len, uint8_t *recv_buffer,  size_t recv_len)
{
    uint32_t tran_len = 0, wait_timeout_10us = 5, timeout_cnt = 0;
    bool busy = false;
    
    printf_note("send: ");
    _print_array(send_buffer, send_len);
    io_reg_rf_set_ndata(send_buffer, send_len);
    tran_len = (send_len&0xff)|((recv_len << 8)&0xff00);
    printf_note("start transfer len:0x%x\n", tran_len);
    io_reg_rf_start_tranfer(tran_len);
    do{
        usleep(10);
        busy = io_reg_rf_is_busy();
        if(timeout_cnt++ >= wait_timeout_10us)
            break;
    }while(busy);

    if(timeout_cnt > wait_timeout_10us){
        printf_warn("after %dus[cnt:%d], read rf busy status timeout\n", timeout_cnt*wait_timeout_10us, timeout_cnt);
        return -1;
    }
    /* GET SPI data */
    io_reg_rf_get_data(recv_buffer, recv_len);
    printf_note("recv: ");
    _print_array(recv_buffer, recv_len);
    
    if(_spi_data_check(recv_buffer, recv_len) == false){
        printf_warn("recv spi data is not valid\n");
        return -1;
    }
    
    return 0;
}

static inline int _get_module_index(int raw_code)
{
    for(int i = 0; i < ARRAY_SIZE(module_info); i ++){
        if(raw_code == module_info[i].code)
            return module_info[i].index;
    }
    return -1;
}

static inline char *_get_module_name(int index)
{
    for(int i = 0; i < ARRAY_SIZE(module_info); i ++){
        if(index == module_info[i].index)
            return module_info[i].name;
    }
    return NULL;
}


static void _spi_info_load(void *info, void *recv_buffer, int recv_size)
{
    struct rf_identity_info_t *_recv_buffer = (struct rf_identity_info_t *)recv_buffer;
    struct rf_info_t *_info = (struct rf_info_t *)info;
    char buffer[128] = {0};

    if(_info->is_load_info == true){
        safe_free(_info->fw_version);
        safe_free(_info->father_version);
        safe_free(_info->protocol_version);
        printf_warn("overlap info\n");
    }

    if((_info->module  = _get_module_index(_recv_buffer->module_code)) == -1){
        printf_warn("module code err:0x%x\n", _recv_buffer->module_code);
    }
    _info->serial_num =  _recv_buffer->serial_num;
    _info->addr = (_recv_buffer->addr_ch >> 4) & 0x0f;
    _info->ch = _recv_buffer->addr_ch  & 0x0f;
    _info->vender = _recv_buffer->vender;
    _info->max_clk_mhz = _recv_buffer->max_clk_mhz;
    
    RF_FW_VERSION_TO_STR(buffer, _recv_buffer->fw_version);
    _info->fw_version = safe_strdup(buffer);

    RF_FW_VERSION_TO_STR(buffer, _recv_buffer->father_version);
    _info->father_version = safe_strdup(buffer);

    RF_FW_VERSION_TO_STR(buffer, _recv_buffer->protocol_version);
    _info->protocol_version = safe_strdup(buffer);

    _info->is_load_info = true;
    
    printfn("=========Rf Info=============================\n");
    printfn("module: %s\n", _get_module_name(_info->module));
    printfn("ch: 0x%x\n", _info->ch);
    printfn("serial_num: 0x%x\n", _info->serial_num);
    printfn("addr: 0x%x\n", _info->addr);
    if(_info->vender >= sizeof(vendor_code_str)/sizeof(*vendor_code_str))
        printfn("vender: 0x%x(%s)\n", _info->vender,"Unkonwn");
    else
        printfn("vender: 0x%x(%s)\n", _info->vender,vendor_code_str[_info->vender]);
    
    RF_MAXCLK_TO_STR(buffer, _info->max_clk_mhz);
    printfn("max_clk_mhz: 0x%x(%s)\n", _info->max_clk_mhz,buffer);
    printfn("fw_version: %s\n", _info->fw_version);
    printfn("father_version: %s\n", _info->father_version);
    printfn("protocol_version: %s\n", _info->protocol_version);
    printfn("==============================================\n");
}


static int _spi_assemble_send_data(uint8_t *buffer, uint8_t mcode, uint8_t addr, uint8_t ch, uint8_t icode, 
                                            uint8_t ex_icode, uint8_t *pdata, size_t data_len)
{
    uint8_t *ptr = buffer;
    uint8_t *pcheck = NULL;
    if(ptr == NULL)
        return -1;
    
    *ptr++ = mcode;
    pcheck = ptr;
    *ptr++ = ((addr << 4) & 0xf0) | ch;
    *ptr++ = icode;
    *ptr++ = ex_icode;
    *ptr++ = data_len;
    if(pdata){
        memcpy(ptr, pdata, data_len);
    }
    ptr += data_len;
    *ptr++ = _spi_checksum(pcheck, data_len);

    return (ptr - buffer);/* RETURN: total frame byte count */
}


static int _refill_recv_data_test(uint8_t *buffer)
{
    uint8_t *ptr = buffer;
    //00 73 a1 a1 05 00 d5 02 b5 a9 00 00
    *ptr++ = 0x00;
    *ptr++ = 0x00;
    *ptr++ = 0xa9;
    *ptr++ = 0xb5;
    *ptr++ = 0x02;
    *ptr++ = 0xd5;
    *ptr++ = 0x00;
    *ptr++ = 0x05;
    *ptr++ = 0xa1;
    *ptr++ = 0xa1;
    *ptr++ = 0x73;
    *ptr++ = 0x00;
    return (ptr - buffer);
}


static int _get_rf_identify_info(uint8_t ch, void *info)
{
    uint8_t frame_buffer[128], recv_buffer[128];
    int frame_size,recv_size =12, ret = 0;
    
    for(int index = 0; index < MAX_XDMA_RF_CARD_NUM; index++){
        for(int module = 0; module < ARRAY_SIZE(module_info); module ++){
            frame_size = _spi_assemble_send_data(frame_buffer, module_info[module].code, index, ch, INS_CODE_IDENTITY, 
                            EX_ICMD_NO_RESP, NULL, 0);
            if(_spi_send_data_by_reg(frame_buffer, frame_size, recv_buffer, recv_size) == 0){
                _spi_info_load(&rf_info[index], recv_buffer, recv_size);
                ret++;
                break;
            }
        }
    }

    if(ret > 0 && info != NULL)
        info = rf_info;
    
    return ret;
}

static void *_rf_get_info(void)
{
   return rf_info;
}

static bool check_rf_status(uint8_t ch)
{
    int ret = 0;
    ret = _get_rf_identify_info(ch,  NULL);
    return (ret > 0 ? true : false);
}

static int _rf_init(void)
{
    for(int i = 0; i < MAX_XDMA_RF_CARD_NUM; i ++){
        memset(&rf_info[i], 0, sizeof(struct rf_info_t));
    }
    return 0;
}

static const struct rf_ops _rf_ops = {
    .init = _rf_init,
    .get_status     = check_rf_status,
    .get_rf_identify_info = _get_rf_identify_info,
    .get_info = _rf_get_info,
};


struct rf_ctx * rf_create_context(void)
{
    int ret = -1;
    struct rf_ctx *ctx = calloc(1, sizeof(*ctx));
     if (!ctx)
        return NULL;
     
    ctx->ops = &_rf_ops;
    ret = _rf_init();
    
    if(ret != 0)
        return NULL;
    return ctx;
}

