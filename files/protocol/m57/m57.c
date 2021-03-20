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
#include "config.h"
#include "m57.h"


void _m57_swap_header_element(struct m57_header_st *header)
{
    uint16_t _tmp = header->number;

    header->number = (header->type << 4) | header->prio;
    header->type = _tmp &0x0ff;
    header->prio = (_tmp >> 4) &0x0ff;
    
    printf_note("after swap type = 0x%x\n", header->type);
    printf_note("prio = 0x%x\n", header->prio);
    printf_note("number = 0x%x\n", header->number);
    printf_note("len = 0x%x\n", header->len);
}

/*
功能： 控制/数据协议头解析
输入参数
    @client: client info
    @buf: 接收缓存指针
    @len: 接收缓存数据长度
输出参数
    @head_len: 返回协议头长度
    @code： 返回错误码
    RETURN: 
        false:解析错误
        true： 解析成功
*/
bool m57_parse_header(void *client, const char *buf, int len, int *head_len, int *code)
{
    struct net_tcp_client *cl = client;
    struct m57_header_st *header;
    void *ex_header;
    int ex_header_len = 0, _header_len = 0, _code = 0;
    int hlen = 0, i, data_len = 0;
    hlen = sizeof(struct m57_header_st);

    _code = C_RET_CODE_SUCCSESS;
    _header_len = len;
    if(len < hlen){
        _code = C_RET_CODE_HEADER_ERR;
        printf_err("rev data len too short: %d\n", len);
        goto exit;
    }

    header = (struct m57_header_st *)buf;
    if(header->header != M57_SYNC_HEADER){
        _code = C_RET_CODE_HEADER_ERR;
        printf_err("header err:%x\n", header->header);
        goto exit;
    }
    printf_note("size = %ld, %ld\n", sizeof(struct m57_header_st), sizeof(struct m57_ex_header_cmd_st));
    printf_note("header = 0x%x\n", header->header);
    printf_note("type = 0x%x\n", header->type);
    printf_note("prio = 0x%x\n", header->prio);
    printf_note("number = 0x%x\n", header->number);
    
    _m57_swap_header_element(header);
    cl->request.data_type = header->type;
    if(header->type == M57_CMD_TYPE){ /* 命令包 */
        printf_note("cmd type\n");
        ex_header_len = sizeof(struct m57_ex_header_cmd_st);
        ex_header = calloc(1, ex_header_len);
        if (!ex_header) {
            printf_err("calloc\n");
            exit(1);
        }
        /* 拷贝命令头 */
        memcpy(ex_header, buf + hlen, ex_header_len);
        cl->request.header = ex_header;
        
        /* 命令长度 = 协议头长度 + 扩展命令头长度 */
        _header_len= hlen + ex_header_len;
        
        /* 命令内容长度 */
        data_len = header->len - hlen - ex_header_len;
        if(data_len >= 0){
            cl->request.content_length = data_len;
        }
    }else if(header->type == M57_DATA_TYPE){    /* 数据包 */
         printf_note("data type\n");
        cl->request.header = NULL;
        /* 命令长度 = 协议头长度 */
        _header_len = hlen;
        /* 数据内容长度 */
        data_len = header->len - hlen;
        if(data_len >= 0){
            cl->request.content_length = data_len;
        }
    } else{
        _code = C_RET_CODE_HEADER_ERR;
        printf_err("unknow type:%x\n", header->type);
        goto exit;
    }
    
exit:
    *code = _code;
    *head_len = _header_len;
    return (_code == C_RET_CODE_SUCCSESS ? true : false);
}


bool m57_execute_data(void *client, int *code)
{
    int err_code = C_RET_CODE_SUCCSESS;
    struct net_tcp_client *cl = client;
    char *payload = (char *)cl->dispatch.body;
    int payload_len = cl->request.content_length;
    /*  TODO */
    printf_note("receive data[%d]...\n", payload_len);
    
    return true;
}

/*
功能： 控制协议执行
输入参数
    @client: client info
输出参数
    @code： 返回错误码
    RETURN: 
        false:执行错误
        true： 执行成功
*/
bool m57_execute_cmd(void *client, int *code)
{
    int err_code = C_RET_CODE_SUCCSESS;
    struct net_tcp_client *cl = client;
    char *payload = (char *)cl->dispatch.body;
    int payload_len = cl->request.content_length;

    struct m57_ex_header_cmd_st *header;
    header = (struct m57_ex_header_cmd_st *)cl->request.header;
    printf_note("receive cmd[0x%x], payload_len=%d\n", header->cmd, payload_len);
    
    switch (header->cmd)
    {
        case CCT_BEAT_HART:
        {
            uint16_t beat_count = 0;
            uint16_t beat_status = 0;
            beat_count = *(uint16_t *)payload;
            beat_status = *((uint16_t *)payload + 1);
            printf_note("keepalive beat_count=%d, beat_status=%d\n", beat_count, beat_status);
            update_tcp_keepalive(cl);
            break;
        }
        case CCT_REGISTER:
        {   
            struct register_st{
                uint8_t type;
                uint8_t type_mode;
                uint8_t client_id[36];
                uint16_t end;
            }__attribute__ ((packed));
            struct register_st _reg;
            #if 0
            data_status_type _type;
            memcpy(&_reg, sizeof(_reg), payload);
            
            _type = (_reg.type == 1 ? TYPE_NORMAL: TYPE_URGENT);
            cl->srv->sub_act->sub(cl->srv->sub_act, cl, _type, -1, NULL);
            #endif
            break;
        }
        case CCT_DATA_SUB:
        {
            struct sub_st{
                uint16_t chip_addr;
                uint16_t client_addr;       //??
                uint16_t port;
            }__attribute__ ((packed));
            struct sub_st _sub;
            memcpy(&_sub,  payload, sizeof(_sub));
            #if 0
            struct net_tcp_server *srv = (struct net_tcp_server *)net_get_data_srv_ctx();
            if(srv == NULL){
                printf_warn("No srv\n");
                err_code = C_RET_CODE_INTERNAL_ERR;
                break;
            }
            char *ip;
            struct net_tcp_client *data_cl;
            
            data_cl = tcp_get_client_by_ipaddr(srv, ip, _sub.port);
            if(data_cl == NULL){
                printf_warn("Not find ip: %s:%d is not connect\n", ip, _sub.port);
                err_code = C_RET_CODE_PARAMTER_ERR;
                break;
            }

            if(_sub.port > 0){
                if(srv->sub_act)
                    srv->sub_act->sub(srv->sub_act, data_cl, -1, _sub.chip_addr, NULL);
            } else{
                for_each_client(srv, ip){
                    if(srv->sub_act)
                        srv->sub_act->sub(srv->sub_act, cl_list, -1, _sub.chip_addr, NULL);
                }
            }
            #endif
            break;
        }
        case CCT_DATA_UNSUB:
        {
            struct unsub_st{
                uint16_t chip_addr;
                uint16_t client_addr;
                uint16_t port;
            }__attribute__ ((packed));
            struct unsub_st _unsub;
            memcpy(&_unsub,  payload, sizeof(_unsub));
            #if 0
            struct net_tcp_server *srv = (struct net_tcp_server *)net_get_data_srv_ctx();
            if(srv == NULL){
                printf_warn("No srv\n");
                err_code = C_RET_CODE_INTERNAL_ERR;
                break;
            }
            
            char *ip;
            struct net_tcp_client *data_cl;
            data_cl = tcp_get_client_by_ipaddr(srv, ip, _unsub.port);
            if(data_cl == NULL){
                printf_warn("Not find ip: %s:%d is not connect\n", ip, _unsub.port);
                err_code = C_RET_CODE_PARAMTER_ERR;
                break;
            }

            if(_unsub.port > 0){
                if(srv->sub_act)
                    srv->sub_act->unsub(srv->sub_act, data_cl, -1, _unsub.chip_addr, NULL);
            } else{
                for_each_client(srv, ip){
                    if(srv->sub_act)
                        srv->sub_act->unsub(srv->sub_act, cl_list, -1, _unsub.chip_addr, NULL);
                }
            }
            #endif
            break;
        }
    }
    return true;
}

bool m57_execute(void *client, int *code)
{
    struct net_tcp_client *cl = client;
    
    if(cl->request.header == NULL)
        return m57_execute_data(client, code);
    
    return m57_execute_cmd(client, code);
}


void m57_send_cmd(void *client, int code, void *args)
{
    
}

void m57_send_error(void *client, int code, void *args)
{

}

