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
#include "../../net/net_sub.h"

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

    printf_debug("type = 0x%x\n", header->type);
    printf_debug("prio = 0x%x\n", header->prio);
    printf_debug("number = 0x%x\n", header->number);

    header->number = header->type | (header->prio << 4);
    header->type = (_tmp >> 4) &0x0ff; 
    header->prio = _tmp &0x0ff;
    
    printf_debug("after swap type = 0x%x\n", header->type);
    printf_debug("prio = 0x%x\n", header->prio);
    printf_debug("number = 0x%x\n", header->number);
}


int _recv_file_over(struct net_tcp_client *cl, char *file, int file_len)
{

    if(cl->section.file.fd > 0){
        #if 1
        int rc = write(cl->section.file.fd, file, file_len);
        if (rc < 0){
            perror("write file");
            return -1;
        }
        #endif
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
    #define _LOAD_TMP_DIR "tmp"

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
        printf_debug("content_length: %d\n", cl->request.content_length);
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
    int _code = -1;
    /*  TODO */
    printf_debug("receive data[%d]...\n", payload_len);
    *code = _code;
    return true;
}

static void _m57_assamble_loadfile_cmd(uint16_t chip_id, uint8_t cmd, uint8_t *buffer)
{
    uint8_t *ptr = NULL, *ptr2 = NULL;
    ptr2 = ptr = buffer;
    *ptr++ = 0x7e;
    *ptr++ = 0xe7;
    *ptr++ = 0xaa;
    *ptr++ = 0x55;
    ptr += 5;
    *(uint16_t *)ptr = chip_id;
    ptr += 2;
    *ptr++ = cmd;
    //ptr += 5;
    *ptr++ = 0x7e;
    *ptr++ = 0xe7;
    *ptr++ = 0xaa;
    *ptr++ = 0x55;
    for(int i = 0; i < ptr - ptr2; i++)
        printfi("%02x ", buffer[i]);
    printfi("\n");
}


static int _m57_start_load_bitfile_to_fpga(uint16_t chip_id)
{
    uint8_t buffer[16];
    struct spm_context *_ctx;
    ssize_t nwrite;
    
    _ctx = get_spm_ctx();
    memset(buffer, 0, sizeof(buffer));
    printfn("Start Load BitFile [fpga id:0x%x]\n", chip_id);
    _m57_assamble_loadfile_cmd(chip_id, 0x00, buffer);
    nwrite = _ctx->ops->write_xdma_data(0, buffer, sizeof(buffer));
    printfn("Start Load %s![%ld]\n",  (nwrite == sizeof(buffer)) ? "OK" : "Faild", nwrite);
    
    return nwrite;
}
static void* _m57_loading_bitfile_to_fpga_thread(void   *args)
{
    #define LOAD_BITFILE_SIZE	4096
    struct stat fstat;
    int result = 0, rc, ret = 0;
    unsigned char buffer[LOAD_BITFILE_SIZE];
    //unsigned char *buffer;
    struct spm_context *_ctx;
    ssize_t nwrite, total_write = 0;;
    FILE *file;
    char *filename = strdup(args);
    #if 0
    posix_memalign((void **)&buffer, 4096 /*alignment */ , LOAD_BITFILE_SIZE + 4096);
    if (!buffer) {
    	fprintf(stderr, "OOM %lu.\n", LOAD_BITFILE_SIZE + 4096);
    	rc = -ENOMEM;
    	return -1;
    }
    #endif
    
    _ctx = get_spm_ctx();
    result = stat(filename, &fstat);
    if(result != 0){
        perror("Faild!");
        ret = -1;
        goto exit;
    }
    printf_info("loading file:%s, size = %ld\n", filename, fstat.st_size);
    
    file = fopen(filename, "r");
    if(!file){
        printf("Open file error!\n");
        ret = -1;
        goto exit;
    }
    rc = fread(buffer, 1, LOAD_BITFILE_SIZE, file);
    printfn("Load Bitfile:[%s]\n", filename);
    while(rc){
        nwrite = _ctx->ops->write_xdma_data(0, buffer, rc);
        total_write += nwrite;
        rc = fread(buffer, 1, LOAD_BITFILE_SIZE, file);
        printfn("Loading.......................................[%ld, %ld]%ld%%\r",  total_write,fstat.st_size, (total_write*100)/fstat.st_size);
    }
    printfn("\nLoad Bitfile %s!\n",  (total_write == fstat.st_size) ? "OK" : "Faild");
    fclose(file);
exit:
    free(filename);
    pthread_exit((void *)ret);
}

static bool   _m57_loading_bitfile_to_fpga(char *filename)
{
    int ret;
    void *status;
    pthread_t work_id;
    
    ret=pthread_create(&work_id, NULL, _m57_loading_bitfile_to_fpga_thread, (void *)filename);
    if(ret!=0){
        perror("pthread cread check");
        return -1;
    }
    pthread_join(work_id, &status);
    ret = (int)status;
    printf_info("write: %s Over, ret = %d\n", filename, ret);
    
    if(ret != 0)
        return false;
    
    return true;
}



static bool  _m57_stop_load_bitfile_to_fpga(uint16_t chip_id)
{
    uint8_t buffer[16];
    struct spm_context *_ctx;
    ssize_t nwrite;
    
    _ctx = get_spm_ctx();
    memset(buffer, 0, sizeof(buffer));
    printfn("Stop Load BitFile [fpga id:0x%x]\n", chip_id);
    _m57_assamble_loadfile_cmd(chip_id, 0x01, buffer);
    nwrite = _ctx->ops->write_xdma_data(0, buffer, sizeof(buffer));
    printfn("Stop Load %s![%ld]\n", (nwrite == sizeof(buffer)) ? "OK" : "Faild", nwrite);
    
    if(nwrite != sizeof(buffer))
        return false;
        
    return true;
}


static int _m57_start_unload_bitfile_from_fpga(uint16_t chip_id)
{
    uint8_t buffer[16];
    struct spm_context *_ctx;
    ssize_t nwrite;
    
    _ctx = get_spm_ctx();
    memset(buffer, 0, sizeof(buffer));
    printfn("Unload BitFile [fpga id:0x%x]\n", chip_id);
    _m57_assamble_loadfile_cmd(chip_id, 0x02, buffer);
    nwrite = _ctx->ops->write_xdma_data(0, buffer, sizeof(buffer));
    printfn("Unload %s![%ld]\n",  (nwrite == sizeof(buffer)) ? "OK" : "Faild", nwrite);
    
    return nwrite;
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
    const char const *payload = (char *)cl->dispatch.body + ex_header_len;
    int payload_len = cl->request.content_length - ex_header_len;
    int _code = -1;

    struct m57_ex_header_cmd_st *header;
    header = (struct m57_ex_header_cmd_st *)cl->request.header;
    printf_info("receive cmd[0x%x], payload_len=%d\n", header->cmd, payload_len);
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
            //update_tcp_keepalive(cl);
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
            
            m57_prio_type _type;
            memcpy(&_reg, payload, sizeof(_reg));
            _type = (_reg.type == 1 ? M57_PRIO_LOW: M57_PRIO_URGENT);
            net_hash_add(cl->section.hash, _type, RT_PRIOID);
            break;
        }
        case CCT_DATA_SUB:
        {
            struct sub_st{
                uint16_t chip_id;
                uint16_t func_id;
                uint16_t port;
            }__attribute__ ((packed));
            struct sub_st _sub;
            memcpy(&_sub,  payload, sizeof(_sub));
            printf_info("sub chip_id:0x%x, func_id=0x%x, port=0x%x\n", _sub.chip_id, _sub.func_id, _sub.port);
            #if 0
            net_hash_add(cl->section.hash, _sub.chip_id, RT_CHIPID);
            net_hash_add(cl->section.hash, _sub.func_id, RT_FUNCID);
            net_hash_add(cl->section.hash, _sub.port, RT_PORTID);
            net_hash_dump(cl->section.hash);
            net_hash_find_type_set(cl->section.hash, RT_CHIPID, NULL);
            #endif
            break;
        }
        case CCT_DATA_UNSUB:
        {
             struct sub_st{
                uint16_t chip_id;
                uint16_t func_id;
                uint16_t port;
            }__attribute__ ((packed));
            struct sub_st _sub;
            memcpy(&_sub,  payload, sizeof(_sub));
            printf_note("unsub chip_id:0x%x, func_id=0x%x, port=0x%x\n", _sub.chip_id, _sub.func_id, _sub.port);
            #if 0
            net_hash_del(cl->section.hash, _sub.chip_id, RT_CHIPID);
            net_hash_del(cl->section.hash, _sub.func_id, RT_FUNCID);
            net_hash_del(cl->section.hash, _sub.port, RT_PORTID);
            net_hash_dump(cl->section.hash);
            #endif
            break;
        }
        case CCT_LIST_INFO:
        {
            uint8_t *info;
            int nbyte = 0;
            nbyte = _reg_get_fpga_info(get_fpga_reg(), 0, (void **)&info);
            if(nbyte >= 0){
                cl->response.data = info;
                cl->response.response_length = nbyte;
                cl->response.prio = M57_PRIO_URGENT;
            }
            _code = CCT_RSP_LIST_INFO;
            break;
        }
        case CCT_READ_BOARD_INFO:
        {
            uint8_t *info;
            int nbyte = 0, h_nbyte;
            struct _req{
                int16_t chipid;
                int16_t fmcid; 
            }__attribute__ ((packed));
            struct _resp{
                int16_t chipid;
                int16_t fmcid; 
                int32_t ret;
            }__attribute__ ((packed));
            struct _req *req = (struct _req *)payload;
            struct _resp *resp;

            nbyte = _reg_get_board_info(0, (void **)&info);
            _code = CCT_RSP_READ_BOARD_INFO;
            
            h_nbyte = nbyte + sizeof(struct _resp);
            if ((resp = safe_malloc(h_nbyte)) == NULL){
              printf_err("Unable to malloc response buffer  %ld bytes!", nbyte + sizeof(struct _resp));
              break;
            }
            memcpy((uint8_t *)resp + sizeof(struct _resp), info, nbyte);
            resp->chipid = req->chipid;
            resp->fmcid = req->fmcid;
            resp->ret = 0;
            if(h_nbyte > 0){
                cl->response.data = resp;
                cl->response.response_length = h_nbyte;
                cl->response.prio = M57_PRIO_URGENT;
            }
            safe_free(info);
            break;
        }
        case CCT_UNLOAD:
        {
            int h_nbyte;
            struct _resp{
                int16_t chipid;
                int32_t ret;
            }__attribute__ ((packed));
            uint16_t id;
            struct _resp *resp;
            h_nbyte = sizeof(struct _resp);
            if ((resp = safe_malloc(h_nbyte)) == NULL){
              printf_err("Unable to malloc response buffer  %ld bytes!",  sizeof(struct _resp));
              break;
            }
            id = *(uint16_t*)payload;
            _m57_start_unload_bitfile_from_fpga(id);
            resp->chipid = id;
            resp->ret = 0;//_reg_get_unload_result(get_fpga_reg(), id, NULL);
            cl->response.data = resp;
            cl->response.response_length = h_nbyte;
            cl->response.prio = M57_PRIO_URGENT;
            break;
        }
        case CCT_LOAD:
        {
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
                    exit(1);
                    break;
                }
                cl->section.file.fd = fd;
            }
            _m57_start_load_bitfile_to_fpga(cl->section.chip_id);
            printf_info("Prepare to receive file[len:%u]\n", cl->section.file.len);
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
            
            
            if(cl->section.is_run_loadfile == false){
                printf_warn("load cmd is not receive!\n");
                break;
            }
            pinfo = (struct file_xinfo *)payload;
            if(pinfo->sn == 0)
                printf_debug("start transfer file\n");
            else
                printf_debug("transfer file......\n");
            
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
                goto load_file_exit;
            }
            printf_debug("receive file:sn=%d, chipid = %d\n", pinfo->sn, pinfo->chipid);
            file = payload + sizeof(struct file_xinfo);
            file_len = payload_len - sizeof(struct file_xinfo);
            ret = _recv_file_over(cl, file, file_len);
            printf_debug("recv file ret: %d\n", ret);
            if(ret == 0){   /* receive over */
                cl->section.is_run_loadfile = false;
                cl->section.is_loadfile_ok = true;
                if(cl->section.file.fd > 0)
                    close(cl->section.file.fd);
                printf_info("peer addr: %s:%d\n", cl->get_peer_addr(cl), cl->get_peer_port(cl));
                printf_info("receive file ok, path = %s, total size:%u[%u]\n", cl->section.file.path, cl->section.file.len_offset, cl->section.file.len);
            }else if(ret < 0){   /* receive err */
                cl->section.is_run_loadfile = false;
                cl->section.is_loadfile_ok = false;
                if(cl->section.file.fd > 0)
                    close(cl->section.file.fd);
                printf_note("receive file err, path = %s\n", cl->section.file.path);
            }
load_file_exit:
            /* now file receive over! response to client*/
            if(cl->section.is_run_loadfile == false){
                struct _resp{
                    int16_t chipid;
                    int32_t ret; 
                }__attribute__ ((packed));
                struct _resp *r_resp = safe_malloc(sizeof(struct _resp));
                r_resp->chipid = pinfo->chipid;
                if(cl->section.is_loadfile_ok){
                    if(_m57_loading_bitfile_to_fpga(cl->section.file.path) == true){
                         _m57_stop_load_bitfile_to_fpga(cl->section.chip_id);
                    } else{
                        r_resp->ret = -1;
                    }
                   // r_resp->ret = _reg_get_load_result(get_fpga_reg(), cl->section.chip_id, NULL);
                    r_resp->ret = 0; /* ok */
                }
                else
                    r_resp->ret = -1;    /* faild */
                cl->response.data = r_resp;
                cl->response.response_length = sizeof(struct _resp);
                _code = CCT_RSP_LOAD;
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

    //update_tcp_keepalive(cl);
    if(cl->request.data_type == M57_DATA_TYPE)
        return m57_execute_data(client, code);
    else if(cl->request.data_type == M57_CMD_TYPE)
        return m57_execute_cmd(client, code);
    
    return false;
}


void m57_send_cmd(void *client, int code, void *args, uint32_t len)
{
    static uint8_t sn = 0;
    struct net_tcp_client *cl = client;
    struct m57_header_st header;
    struct m57_ex_header_cmd_st ex_header;
    int hlen = sizeof(struct m57_header_st);
    int exhlen = sizeof(struct m57_ex_header_cmd_st);
    uint8_t *psend;
    
    header.header = M57_SYNC_HEADER;
    header.type = M57_CMD_TYPE;
    header.prio = cl->section.prio;
    header.number = sn++;
    header.len = hlen  + exhlen + len;
    _m57_swap_send_header_element(&header);
    ex_header.cmd = code;
    ex_header.len = exhlen + len;

    psend = calloc(1, header.len);
    if (!psend) {
            printf_err("calloc\n");
            exit(1);
        }
    memcpy(psend, &header, hlen);
    memcpy(psend + hlen, &ex_header, exhlen);
    if(len > 0 && args)
        memcpy(psend + hlen + exhlen, args, len);
    
    cl->send(cl, psend, header.len);
#if 0
    for(int i = 0; i < header.len; i++)
        printfn("%02x ", psend[i]);
    printfn("\n");
#endif
    safe_free(psend);
}

void m57_send_heatbeat(void *client)
{
    struct net_tcp_client *cl = client;
    struct _beat_t{
        uint16_t beat_count;
        uint16_t beat_status;
    };
    struct _beat_t beatheart;
    beatheart.beat_count = 0;//cl->section.beatheat ++;
    beatheart.beat_status = 0;
    cl->section.prio = M57_PRIO_LOW;
    printf_debug("send beatheart to:[%s:%d],n=%d \n", cl->get_peer_addr(cl), cl->get_peer_port(cl), beatheart.beat_count);
    m57_send_cmd(client, CCT_BEAT_HART, &beatheart, sizeof(beatheart));
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

    printf_debug("code = [0x%x, %d]\n", code, code);
    memset(&header, 0, sizeof(header));
    header.header = M57_SYNC_HEADER;
    header.type = M57_CMD_TYPE;
    header.prio = cl->response.prio;
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

#if 0
    printfn("send resp[%d]: \n", header.len);
    for(int i = 0; i < header.len; i++)
        printfn("%02x ", psend[i]);
    printfn("\n");
#endif
    
    cl->send(cl, psend, header.len);

    safe_free(psend);
}


void m57_send_error(void *client, int code, void *args)
{
    
}

