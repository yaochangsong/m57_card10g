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
    
    printf_debug("after swap type = 0x%x\n", header->type);
    printf_debug("prio = 0x%x\n", header->prio);
    printf_debug("number = 0x%x\n", header->number);
    printf_debug("len = 0x%x\n", header->len);
}

void _m57_swap_send_header_element(struct m57_header_st *header)
{
    uint16_t _tmp = header->number;

    printf_note("type = 0x%x\n", header->type);
    printf_note("prio = 0x%x\n", header->prio);
    printf_note("number = 0x%x\n", header->number);

    header->number = header->type | (header->prio << 4);
    header->type = (_tmp >> 4) &0x0ff; 
    header->prio = _tmp &0x0ff;
    
    printf_note("after swap type = 0x%x\n", header->type);
    printf_note("prio = 0x%x\n", header->prio);
    printf_note("number = 0x%x\n", header->number);
}


int _recv_file_over(struct net_tcp_client *cl, char *file, int file_len)
{

    if(cl->section.file.fd > 0){
        int rc = write(cl->section.file.fd, file, file_len);
        if (rc < 0){
            perror("write file");
            return -1;
        }
    }else{
        return -1;
    }
        

    cl->section.file.len_offset += file_len;
    if((cl->section.file.len_offset == cl->section.file.len) || cl->section.file.sn == -1){
        if(cl->section.file.len_offset != cl->section.file.len){
            printf_warn("receive len is too short: %d, %u\n", cl->section.file.len_offset, cl->section.file.len);
        }
        if(cl->section.file.sn != -1){
            printf_warn("the sn is wrong:%d\n", cl->section.file.sn);
        }
        /* write over */
        return 0;
    } else if(cl->section.file.len_offset > cl->section.file.len){
        /* err: write too long */
        printf_err("receive data is too long: offset:%d, len: %u\n", cl->section.file.len_offset, cl->section.file.len);
        return -1;
    }
        
    
    return file_len;
}

char *_create_tmp_path(struct net_tcp_client *cl)
{
    #define _LOAD_TMP_DIR "/tmp"

    if(cl->section.file.fd > 0){
        close(cl->section.file.fd);
    }

   // if(strlen(cl->section.file.path) > 0){
   //     printf_note("remove path: %s\n", cl->section.file.path);
   //     remove(cl->section.file.path);
   // }
    memset(cl->section.file.path, 0, sizeof(cl->section.file.path));
    snprintf(cl->section.file.path, sizeof(cl->section.file.path) - 1, "%s/.%d.tmp", _LOAD_TMP_DIR, cl->section.chip_id);
   // if(access(cl->section.file.path, 0) == 0){
   //     printf_note("remove path: %s\n", cl->section.file.path);
   //     remove(cl->section.file.path);
   // }
    printf_debug("path: %s\n", cl->section.file.path);
    return cl->section.file.path;
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
    printf_debug("size = %ld, %ld\n", sizeof(struct m57_header_st), sizeof(struct m57_ex_header_cmd_st));
    printf_debug("header = 0x%x\n", header->header);
    printf_debug("type = 0x%x\n", header->type);
    printf_debug("prio = 0x%x\n", header->prio);
    printf_debug("number = 0x%x\n", header->number);
    
    _m57_swap_header_element(header);
    cl->request.prio = header->prio;
    cl->request.data_type = header->type;
    if(header->type == M57_CMD_TYPE){ /* 命令包 */
        printf_debug("cmd type\n");
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
        _header_len= hlen ;//+ ex_header_len;
        
        /* 命令内容长度 */
        data_len = header->len - hlen;// - ex_header_len;
        if(data_len >= 0){
            cl->request.content_length = data_len;
        }
        printf_debug("len: %d\n", cl->request.content_length);
    }else if(header->type == M57_DATA_TYPE){    /* 数据包 */
         printf_debug("data type\n");
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
    int ex_header_len = sizeof(struct m57_ex_header_cmd_st);
    char *payload = (char *)cl->dispatch.body + ex_header_len;
    int payload_len = cl->request.content_length - ex_header_len;
    int _code = -1;

    struct m57_ex_header_cmd_st *header;
    header = (struct m57_ex_header_cmd_st *)cl->request.header;
    printf_note("receive cmd[0x%x], payload_len=%d\n", header->cmd, payload_len);
    if(payload_len < 0){
        printf_err("recv cmd data len err\n");
    }
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
            printf_note("channel register!!\n");// len: 40
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
        case CCT_LIST_INFO:
        {
            uint8_t *info;
            int nbyte;
            nbyte = _reg_get_fpga_info(0, (void **)&info);
            printf_note("recv get info cmd: %d\n", nbyte);
            if(nbyte > 0){
                cl->response.data = info;
                cl->response.response_length = nbyte;
            }
            _code = CCT_RSP_LIST_INFO;
            break;
        }
        case CCT_LOAD:
        {
            struct load_info{
                uint16_t chipid;
                uint8_t is_reload;
                uint8_t resv;
                uint8_t encry;
                uint8_t resv2;
                uint32_t file_len;
            }__attribute__ ((packed));
            struct load_info *pload;
            int fd;
            
            pload = (struct load_info *)payload;
            cl->section.chip_id = pload->chipid;
            cl->section.file.len = pload->file_len;
            cl->section.file.len_offset = 0;
            cl->section.file.sn = 0;
            cl->section.is_run_loadfile = true;
            cl->section.is_loadfile_ok = false;
            _create_tmp_path(cl);
            if(strlen(cl->section.file.path) > 0){
                fd = open(cl->section.file.path, O_CREAT|O_RDWR|O_TRUNC, S_IWUSR|S_IRUSR);
                if (fd < 0){
                    printf_err("open file failed..!!\n");
                    break;
                }
                cl->section.file.fd = fd;
            }
            printf_debug("Prepare to receive file\n");
                break;
        }
        case CCT_FILE_TRANSFER:
        {
            char *file;
            int file_len;
            int ret, status = 0;
            struct file_xinfo{
                uint16_t chipid;
                int16_t sn;
            }__attribute__ ((packed));
            struct file_xinfo linfo, *pinfo;
            
            printf_debug("start transfer file\n");
            if(cl->section.is_run_loadfile == false){
                printf_warn("load cmd is not receive!\n");
                break;
            }
            pinfo = (struct file_xinfo *)payload;
            if(cl->section.chip_id != pinfo->chipid){
                printf_err("chipid err = %d\n", pinfo->chipid);
                status = -1;
            }
            if(pinfo->sn != -1){
                if(pinfo->sn != cl->section.file.sn){
                    printf_err("sn err = %d, %d\n", pinfo->sn, cl->section.file.sn);
                    status = -1;
                }else{
                    cl->section.file.sn++;
                }
            }else{
                cl->section.file.sn = -1; /* receive over */
            }
            if(status == -1){   /* receive err */
                cl->section.is_run_loadfile = false;
                cl->section.is_loadfile_ok = false;
                printf_err("receive file err\n");
                break;
            }
            printf_debug("receive file:sn=%d, chipid = %d\n", pinfo->sn, pinfo->chipid);
            file = payload + sizeof(struct file_xinfo);
            file_len = payload_len;
            ret = _recv_file_over(cl, file, file_len);
            if(ret == 0){   /* receive over */
                cl->section.is_run_loadfile = false;
                cl->section.is_loadfile_ok = true;
                if(cl->section.file.fd > 0)
                    close(cl->section.file.fd);
                printf_note("receive file ok, path = %s\n", cl->section.file.path);
            }else if(ret < 0){   /* receive err */
                cl->section.is_run_loadfile = false;
                cl->section.is_loadfile_ok = false;
                if(cl->section.file.fd > 0)
                    close(cl->section.file.fd);
                printf_note("receive file err, path = %s\n", cl->section.file.path);
            }
            break;
        }
    }
    
    *code = _code;
    return true;
}

bool m57_execute(void *client, int *code)
{
    struct net_tcp_client *cl = client;

    update_tcp_keepalive(cl);
    if(cl->request.data_type == M57_DATA_TYPE)
        return m57_execute_data(client, code);
    else if(cl->request.data_type == M57_CMD_TYPE)
        return m57_execute_cmd(client, code);
    
    return false;
}


void m57_send_cmd(void *client, int code, void *args)
{
    static uint8_t sn = 0;
    struct net_tcp_client *cl = client;
    struct m57_header_st header;
    struct m57_ex_header_cmd_st ex_header;
    int hlen = sizeof(struct m57_header_st);
    int exhlen = sizeof(struct m57_ex_header_cmd_st);
    uint8_t *psend;
    
    header.header = M57_SYNC_HEADER;
    header.prio = 0x03;
    header.type = 0x02;
    header.number = sn++;
    header.len = hlen  + exhlen + cl->response.response_length;

    ex_header.cmd = code;
    ex_header.len = exhlen + cl->response.response_length;

    psend = calloc(1, header.len);
    if (!psend) {
            printf_err("calloc\n");
            exit(1);
        }
    memcpy(psend, &header, hlen);
    memcpy(psend + hlen, &ex_header, exhlen);
    if(cl->response.response_length > 0 && cl->response.data)
        memcpy(psend + hlen + exhlen, cl->response.data, cl->response.response_length);
    
    cl->send(cl, psend, hlen);

    safe_free(psend);
}

void m57_send_resp(void *client, int code, void *args)
{
    if(code == -1)
            return;

    static uint8_t sn = 0;
    struct net_tcp_client *cl = client;
    struct m57_header_st header;
    struct m57_ex_header_cmd_st ex_header;
    int hlen = sizeof(struct m57_header_st);
    int exhlen = sizeof(struct m57_ex_header_cmd_st);
    uint8_t *psend;

    printf_note("code = [0x%x, %d]\n", code, code);
    memset(&header, 0, sizeof(header));
    header.header = M57_SYNC_HEADER;
    header.prio = 0x03;
    header.type = 0x02;
    header.number = sn++;
    header.len = hlen  + exhlen + cl->response.response_length;
    _m57_swap_send_header_element(&header);

    memset(&ex_header, 0, sizeof(ex_header));
    ex_header.cmd = code;
    ex_header.len = exhlen + cl->response.response_length;

    psend = calloc(1, header.len);
    if (!psend) {
            printf_err("calloc\n");
            return;
        }
    memcpy(psend, &header, hlen);
    memcpy(psend + hlen, &ex_header, exhlen);

    if(cl->response.response_length > 0 && cl->response.data)
        memcpy(psend + hlen + exhlen, cl->response.data, cl->response.response_length);

#if 1
    printfn("send resp[%d]: \n", header.len);
    for(int i = 0; i < header.len; i++)
        printfn("%02x ", psend[i]);
#endif
    
    cl->send(cl, psend, header.len);

    safe_free(psend);
}


void m57_send_error(void *client, int code, void *args)
{
    
}

