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
#include "../../bsp/io.h"

static int _m57_write_data_to_fpga(uint8_t *ptr, size_t len);
static bool  _m57_stop_load_bitfile_to_fpga(uint16_t chip_id);
static void _assamble_resp_payload(struct net_tcp_client *cl, int16_t chipid, int ret);
static int _m57_load_bit_over(void *client, bool result);
static int _m57_stop_load(void *client);
static void print_array(uint8_t *ptr, ssize_t len);


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

/* 为了对密钥软件回复数据，需要分析数据包 */
static bool _m57_parse_keytool_data(void *ptr, size_t len, int *code)
{
        #define _KEYTOOL_ADDR_ID 0x00
        #define _KEYTOOL_READ_TYPE_ID 0x0102
        #define _KEYTOOL_READ_PORT_ID 0x07
        #define _KEYTOOL_READ_FUNC_ID 0x01
        #define _KEYTOOL_READ_LEN 26
        #define _KEYTOOL_WRITE_TYPE_ID 0x0202
        #define _KEYTOOL_WRITE_PORT_ID 0x02
        #define _KEYTOOL_WRITE_FUNC_ID 0x01
        #define _KEYTOOL_WRITE_MIN_LEN 300
//51 57 00 10 1a 00 00 00 00 02 00 00 01 00 0a 00 02 01 01 00 07 00 00 00 00 00
//51 57 02 10 5a 01 00 00 00 03 00 00 01 00 4a 01 02 02 01 00 02 00 00 00  00 00
//write type:0x202, py_src_addr: 0x0, chip_id:0x0, port: 0x2, prio_id:0x1, func_id:0x1, len:346
    uint8_t *_ptr = ptr;
    uint8_t *payload = NULL;
    struct data_frame_t *pdata;
    //struct net_sub_st parg;
    bool ret = false;
    
    if(*(uint16_t *)_ptr != 0x5751){  /* header */
        printf_warn("error header=0x%x\n", *(uint16_t *)_ptr);
        return false;
    }
    payload = _ptr + 8;
    pdata = (struct data_frame_t *)payload;
    //parg.chip_id = pdata->py_src_addr;
    //parg.func_id = pdata->src_addr;
    //parg.prio_id = (*(uint8_t *)(_ptr + 3) >> 4) & 0x0f;
    //parg.port = pdata->port;
    //printf_note("type:0x%x, py_src_addr: 0x%x, chip_id:0x%x, port: 0x%x, prio_id:0x%x, func_id:0x%x, len:%ld\n", pdata->type, pdata->py_src_addr, parg.chip_id, parg.port, parg.prio_id, parg.func_id, len);
    if(pdata->py_src_addr  == _KEYTOOL_ADDR_ID && (pdata->type == _KEYTOOL_WRITE_TYPE_ID || pdata->type == _KEYTOOL_READ_TYPE_ID)){
        if(len ==_KEYTOOL_READ_LEN && pdata->type == _KEYTOOL_READ_TYPE_ID && pdata->port == _KEYTOOL_READ_PORT_ID && pdata->src_addr == _KEYTOOL_READ_FUNC_ID){
            *code = CCT_KEY_READ_E2PROM;
             printf_note("Read EEPROM Cmd\n");
            ret = true;
        } else if(len >= _KEYTOOL_WRITE_MIN_LEN && pdata->type == _KEYTOOL_WRITE_TYPE_ID && pdata->port ==  _KEYTOOL_WRITE_PORT_ID && pdata->src_addr == _KEYTOOL_WRITE_FUNC_ID){
            *code = CCT_KEY_WRITE_E2PROM;
             printf_note("Write EEPROM Cmd\n");
             ret = true;
        }
    }

    return ret;
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
bool m57_parse_header(void *client, char *buf, int len, int *head_len, int *code)
{
    struct net_tcp_client *cl = client;
    struct m57_header_st  hbuffer, *header;
    void *ex_header;
    int ex_header_len = 0, _header_len = 0, _code = 0;
    int hlen = 0, i, data_len = 0;
    hlen = sizeof(struct m57_header_st);

    _code = C_RET_CODE_SUCCSESS;
    cl->request.data_state = NET_TCP_DATA_NORMAL;
    _header_len = len;
    
    if(len < hlen){
        cl->request.data_state = NET_TCP_DATA_WAIT;
        printf_warn("recv data len too short: %d\n", len);
        goto exit;
    }
    
    memcpy(&hbuffer, buf, sizeof(struct m57_header_st));
    header = &hbuffer;
    if(header->header != M57_SYNC_HEADER){
        _code = C_RET_CODE_HEADER_ERR;
        cl->request.data_state = NET_TCP_DATA_ERR;
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

        void *hheader = calloc(1, hlen);
        if (!hheader) {
            printf_err("calloc\n");
            exit(1);
        }
        memcpy(hheader, header, hlen);
        cl->request.header = hheader;
        
        /* 数据内容长度 */
        data_len = header->len - hlen;
        if(data_len >= 0){
            cl->request.content_length = data_len;
        }else{
            printf_err("%d,data_len error\n",data_len);
            _code = C_RET_CODE_HEADER_ERR;
            cl->request.data_state = NET_TCP_DATA_ERR;
            goto exit;
        }
        ns_downlink_add_cmd_pkgs(1);
        _header_len = hlen;     //消费header
        cl->state = NET_TCP_CLIENT_STATE_DATA;
        
    }else if(header->type == M57_DATA_TYPE){    /* 数据包 */
        int code = 0;
        if (len < header->len) {  //必须等到完整数据帧
             cl->request.data_state = NET_TCP_DATA_WAIT;
            printf_warn("recv data packet len too short: %d， %d\n", len, header->len);
            goto exit;
        }
        
        cl->request.header = NULL;
        _header_len = header->len;//消费帧长
        //print_array((void *)buf, len);
        //_m57_parse_keytool_data((void *)buf, _header_len, &code);
        /* 密钥软件下发数据，使用命令函数解析 */
#ifdef KEY_TOOL_ENABLE
        if(_m57_parse_keytool_data((void *)buf, _header_len, &code)){
            cl->state = NET_TCP_CLIENT_STATE_DATA;
            header->resv = code; //使用协议头保留字段承载命令
            *(uint16_t*)(buf + 6) = code;
            _header_len = hlen - sizeof(header->resv);//消费帧长 - 保留字段长度
            cl->request.content_length = data_len;
            cl->section.is_keytool_cmd = true;
            cl->request.data_type = M57_CMD_TYPE;
            data_len = header->len - _header_len;
            //printf_note("_header_len: %x, data_len: %d, cl=%p\n", _header_len, data_len, cl);
            if(data_len >= 0){
                cl->request.content_length = data_len;
            }else{
                printf_err("%d,data_len error\n",data_len);
                _code = C_RET_CODE_HEADER_ERR;
                cl->request.data_state = NET_TCP_DATA_ERR;
            }
            goto exit;
        }
 #endif
        cl->state = NET_TCP_CLIENT_STATE_HEADER;
        /* 数据包不处理透传 */
        _m57_write_data_to_fpga((uint8_t *)buf, _header_len);
        ns_downlink_add_data_pkgs(1);
    } else{
        _header_len = len;
        _code = C_RET_CODE_HEADER_ERR;
        cl->request.data_state = NET_TCP_DATA_ERR;
        printf_err("unknow type:%x\n", header->type);
        goto exit;
    }
    
exit:
    if(_code != C_RET_CODE_SUCCSESS)
        ns_downlink_add_err_pkgs(1);
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
    #define _LOAD_FILE_CMD_LEN 16
    //uint8_t buffer[16];
    void *buffer;
    struct spm_context *_ctx;
    ssize_t nwrite;
    int pagesize = 0;

    pagesize=getpagesize();
    posix_memalign((void **)&buffer, pagesize /*alignment */ , _LOAD_FILE_CMD_LEN + pagesize);
    if (!buffer) {
        fprintf(stderr, "OOM %u.\n", pagesize);
        return -1;
    }
    _ctx = get_spm_ctx();
    memset(buffer, 0, _LOAD_FILE_CMD_LEN);
    printfn("Start Load BitFile [fpga id:0x%x]\n", chip_id);
    _m57_assamble_loadfile_cmd(chip_id, 0x00, buffer);
    nwrite = _ctx->ops->write_xdma_data(0, buffer, _LOAD_FILE_CMD_LEN);
    printfn("Start Load %s![%ld]\n",  (nwrite == _LOAD_FILE_CMD_LEN) ? "OK" : "Faild", nwrite);
    free(buffer);
    buffer = NULL;
    usleep(50000);
    return nwrite;
}

static inline int _align_4byte(int *rc)
{
    int _rc = *rc;
    int reminder = _rc % 4;
    if(reminder != 0){
        _rc += (4 - reminder);
        *rc = _rc;
        return (4 - reminder);
    }
    return 0;
}

static void* _m57_loading_bitfile_to_fpga_thread(void   *args)
{
    #define LOAD_BITFILE_SIZE	4096
    struct stat fstat;
    int result = 0, rc, ret = 0;
    unsigned char *buffer;
    struct spm_context *_ctx;
    ssize_t nwrite, total_write = 0;;
    FILE *file;
    char *filename = strdup(args);
    int reminder = 0;
    //char *filename = "test.bin";
    posix_memalign((void **)&buffer, 4096 /*alignment */ , LOAD_BITFILE_SIZE + 4096);
    if (!buffer) {
        fprintf(stderr, "OOM %u.\n", LOAD_BITFILE_SIZE + 4096);
        rc = -ENOMEM;
        goto exit2;
    }
    memset(buffer, 0, LOAD_BITFILE_SIZE);
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
    reminder = _align_4byte(&rc);
    printfn("Load Bitfile:[%s]\n", filename);
    printfn("Loading.......................................[%ld, %ld]%ld%%\r",  total_write,fstat.st_size, (total_write*100)/fstat.st_size);
    while(rc){
        nwrite = _ctx->ops->write_xdma_data(0, buffer, rc);
        if(nwrite <= 0)
            break;
        total_write += nwrite;
        memset(buffer, 0, LOAD_BITFILE_SIZE + 4096);
        rc = fread(buffer, 1, LOAD_BITFILE_SIZE, file);
        reminder += _align_4byte(&rc);
        printfn("Loading.......................................[%ld, %ld]%ld%%\r",  total_write,fstat.st_size, (total_write*100)/fstat.st_size);
    }
    printfn("\nLoad Bitfile %s,align 4byte reminder=%d,%ld,%ld\n",  (total_write == fstat.st_size+reminder) ? "OK" : "Faild", reminder, total_write,fstat.st_size+reminder);
    fclose(file);
exit:
    free(buffer);
    buffer = NULL;
exit2:
    free(filename);
    pthread_exit((void *)ret);
}

static int _m57_loading_bitfile_to_fpga_thread_asyn(void   *args)
{
    #define LOAD_BITFILE_SIZE	4096
    
    int rc = 0, ret = 0;
    struct asyn_load_ctx_t *_args = args;
    unsigned char *buffer = _args->buffer;
    struct spm_backend_ops *ops = _args->ops;
    FILE *file = _args->file;
    char *filename = _args->filename;
    struct stat *fstat = _args->fstat;
    int reminder = _args->reminder;
    ssize_t nwrite;

    memset(buffer, 0, LOAD_BITFILE_SIZE + 4096);
    rc = fread(buffer, 1, LOAD_BITFILE_SIZE, file);
    reminder += _align_4byte(&rc);
#ifdef DEBUG_TEST
    _args->total_load += rc;
    usleep(1000);
#else
    nwrite = ops->write_xdma_data(0, buffer, rc);
    if(nwrite > 0)
        _args->total_load += nwrite;
#endif
    printfn("Loading.......................................[%ld, %ld]%ld%%\r",  _args->total_load,fstat->st_size,(_args->total_load*100)/(fstat->st_size+reminder));
    _args->reminder = reminder;
    if(rc <= 0){
        _args->result = (_args->total_load == fstat->st_size+reminder ? true : false);
        printfn("\nLoad Bitfile %s,align 4byte reminder=%d,%ld,%ld\n",  _args->result == true ? "OK" : "Faild", reminder, _args->total_load,fstat->st_size+reminder);
        pthread_exit_by_name(_args->filename);
    }
    return 0;
}

static int _m57_load_bit_over(void *client, bool result)
{
    int ret = M57_CARD_STATUS_OK;
    struct net_tcp_client *cl = client;
    int chip_id = cl->section.chip_id;
    if(cl->section.is_exitting == true){
        printf_note("client is exitting\n");
        return -1;
    }
#ifdef DEBUG_TEST
    sleep(3);
    goto exit;
#endif
    //load faild
    if(result == false){
        ret = M57_CARD_STATUS_LOAD_FAILD;
        ns_downlink_set_loadbit_result(CARD_SLOT_NUM(chip_id), DEVICE_STATUS_LOAD_ERR);
        config_set_device_status(DEVICE_STATUS_LOAD_ERR, chip_id);
        goto exit;
    }
    //load ok
    usleep(50000);
    if(_m57_stop_load_bitfile_to_fpga(chip_id) == true){
        sleep(2);
        /* 1st: check load result */
        ret = _reg_get_load_result(get_fpga_reg(), chip_id, NULL);
        ret = (ret == 0 ? M57_CARD_STATUS_OK : M57_CARD_STATUS_LOAD_FAILD); // -5: load faild; 0: ok
        //device load faild
        if(ret != M57_CARD_STATUS_OK){
            ns_downlink_set_loadbit_result(CARD_SLOT_NUM(chip_id), DEVICE_STATUS_LOAD_ERR);
            config_set_device_status(DEVICE_STATUS_LOAD_ERR, chip_id);
        }
        /* 2st: check link result,
            NOTE:
            1) link switch is on;
            2) bit file load ok
            3) link status noly valid for chip2 
        */
        if(config_get_link_switch(CARD_SLOT_NUM(chip_id)) && ret == 0 && (CARD_CHIP_NUM(chip_id) == 2)){
            ret = _reg_get_link_result(get_fpga_reg(), chip_id, NULL);
            ret = (ret == 0 ? M57_CARD_STATUS_OK : M57_CARD_STATUS_LINK_FAILD); // -7: link faild; 0: ok
            
            //device link faild
            if(ret != M57_CARD_STATUS_OK){
                ns_downlink_set_link_result(CARD_SLOT_NUM(chip_id), DEVICE_STATUS_LINK_ERR);
                config_set_device_status(DEVICE_STATUS_LINK_ERR, chip_id);
            }else{
                ns_downlink_set_link_result(CARD_SLOT_NUM(chip_id), DEVICE_STATUS_LINK_OK);
            }
        }
        if(ret == M57_CARD_STATUS_OK){
            //device load ok
            ns_downlink_set_loadbit_result(CARD_SLOT_NUM(chip_id), DEVICE_STATUS_LOAD_OK);
            config_set_device_status(DEVICE_STATUS_LOAD_OK, chip_id);
        }
        sleep(2);
    }
exit:
    pthread_mutex_lock(&cl->section.free_lock);
    if(cl->section.is_exitting == false){
        _assamble_resp_payload(cl, chip_id, ret);
        cl->srv->send_error(cl, CCT_RSP_LOAD, NULL);
        printf_note("Send Over#\n");
    }
    pthread_mutex_unlock(&cl->section.free_lock);
    
    return 0;
}

static int _m57_asyn_load_exit(void *args)
{
    struct asyn_load_ctx_t *_args = args;
    printf_note("result = %s\n", _args->result == true ? "OK" : "False");
    uloop_timeout_cancel(&_args->timeout);
    printf_note("cancel timer OK\n");
    if(_args->client)
        _m57_load_bit_over(_args->client, _args->result);
    if(_args->file){
        fclose(_args->file);
        _args->file = NULL;
    }
    _safe_free_(_args->buffer);
    _safe_free_(_args->fstat);
    _safe_free_(_args->filename);
    _safe_free_(_args);
    printf_note("FREE & Exit OK!!\n");
    return 0;
}
static void _loadbit_timeout_cb(struct uloop_timeout *timeout)
{
    struct asyn_load_ctx_t *load = container_of(timeout, struct asyn_load_ctx_t, timeout);
    printf_warn("\nLoadBit Timeout...\n");
    _m57_stop_load(load->client);
}


static int _m57_asyn_loadbit_init(void *args)
{
     #define LOAD_BITFILE_SIZE	4096
    struct asyn_load_ctx_t *_args = args;
    struct stat *fstat = safe_malloc(sizeof(*fstat));
    int result = 0, ret = 0;
    unsigned char *buffer = _args->buffer;
    struct spm_context *_ctx;
    ssize_t nwrite, total_write = 0;;
    FILE *file;
    char *filename = _args->filename;
    int reminder = 0;
    //char *filename = "test.bin";
    posix_memalign((void **)&buffer, 4096 /*alignment */ , LOAD_BITFILE_SIZE + 4096);
    if (!buffer) {
        fprintf(stderr, "OOM %u.\n", LOAD_BITFILE_SIZE + 4096);
        //rc = -ENOMEM;
        goto exit;
    }
    memset(buffer, 0, LOAD_BITFILE_SIZE);
    
    result = stat(filename, fstat);
    if(result != 0){
        perror("Faild!");
        ret = -1;
        goto exit;
    }
    printf_info("loading file:%s, size = %ld\n", filename, fstat->st_size);
    
    file = fopen(filename, "r");
    if(!file){
        printf("Open file error!\n");
        ret = -1;
        goto exit;
    }
    _args->file = file;
    _args->ops = get_spm_ctx()->ops;
    _args->fstat = fstat;
    _args->buffer = buffer;
    _args->total_load = 0;
    _args->timeout.cb = _loadbit_timeout_cb;
    uloop_timeout_set(&_args->timeout, LOAD_BITFILE_TIMEOUT * 1000);
    usleep(10);
    printfn("Load Bitfile:[%s]\n", filename);
    printfn("Loading.......................................[%ld, %ld]%ld%%\r",  total_write,fstat->st_size, (total_write*100)/fstat->st_size);
    return ret;
exit:
    _args->result = false;
    pthread_exit_by_name(_args->filename);
    return ret;
}


static int   _m57_loading_bitfile_to_fpga_asyn(void *client, char *filename)
{
    int ret;
    pthread_t tid = 0;
    struct asyn_load_ctx_t *args = safe_malloc(sizeof(*args));
    memset(args, 0 , sizeof(*args));
    args->filename = safe_strdup(filename);
    args->client = client;
    args->result = false;
    struct net_tcp_client *cl = args->client;
    cl->section.load_bit_thread = args;

    ret =  pthread_create_detach (NULL,_m57_asyn_loadbit_init, 
                                 _m57_loading_bitfile_to_fpga_thread_asyn, 
                                 _m57_asyn_load_exit,  
                                args->filename, args, args, &tid);
    if(ret != 0){
        perror("pthread loadbit thread!!");
        safe_free(args->filename);
        safe_free(args);
        return -1;
    }
    printf_note(">>>>>>>tid:%lu\n", tid);
    args->tid = tid;
    
    return 0;
}

int _m57_stop_load(void *client)
{
    struct net_tcp_client *cl = client;

    if(cl == NULL || cl->section.load_bit_thread == NULL)
        return -1;
        
    printf_note("cl->section.load_bit_thread tid: %ld\n", cl->section.load_bit_thread->tid);
    if(pthread_check_alive_by_tid(cl->section.load_bit_thread->tid) == true){
        pthread_cancel_by_tid(cl->section.load_bit_thread->tid);
        printf_note(">>>>>>>>>>STOP LoadBit: %s\n", cl->section.load_bit_thread->filename);
    }
    usleep(10);
    return 0;
}


int m57_stop_load(void *client)
{
#if LOAD_FILE_ASYN
    _m57_stop_load(client);
#endif
    return 0;
}

int   m57_loading_bitfile_to_fpga_asyn(void *client, char *filename)
{
    return  _m57_loading_bitfile_to_fpga_asyn(client, filename);
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
     #define _STOP_LOAD_FILE_CMD_LEN 16
    //uint8_t buffer[16];
    void *buffer = NULL;
    struct spm_context *_ctx;
    ssize_t nwrite;
     int pagesize = 0;

    pagesize=getpagesize();
    posix_memalign((void **)&buffer, pagesize /*alignment */ , _STOP_LOAD_FILE_CMD_LEN + pagesize);
    if (!buffer) {
        fprintf(stderr, "OOM %u.\n", pagesize);
        return false;
    }
    _ctx = get_spm_ctx();
    memset(buffer, 0, _STOP_LOAD_FILE_CMD_LEN);
    printfn("Stop Load BitFile [fpga id:0x%x]\n", chip_id);
    _m57_assamble_loadfile_cmd(chip_id, 0x01, buffer);
    nwrite = _ctx->ops->write_xdma_data(0, buffer, _STOP_LOAD_FILE_CMD_LEN);
    printfn("Stop Load %s![%ld]\n", (nwrite == _STOP_LOAD_FILE_CMD_LEN) ? "OK" : "Faild", nwrite);

    free(buffer);
    buffer = NULL;
    if(nwrite != _STOP_LOAD_FILE_CMD_LEN){
        return false;
    }

    return true;
}


static int _m57_start_unload_bitfile_from_fpga(uint16_t chip_id)
{
    uint8_t buffer[16];
    struct spm_context *_ctx;
    ssize_t nwrite, ret = 0;
    
    _ctx = get_spm_ctx();
    memset(buffer, 0, sizeof(buffer));
    printfn("Unload BitFile [fpga id:0x%x]\n", chip_id);
    _m57_assamble_loadfile_cmd(chip_id, 0x02, buffer);
    nwrite = _ctx->ops->write_xdma_data(0, buffer, sizeof(buffer));
    ret = _reg_get_unload_result(get_fpga_reg(), chip_id, NULL);
    if(ret == 0 && (nwrite == sizeof(buffer))){
        printfn("Unload[fpgaId:0x%x] OK!\n",  chip_id);
        ns_downlink_set_loadbit_result(CARD_SLOT_NUM(chip_id), DEVICE_STATUS_WAIT_LOAD);
    } else{
        printfn("Unload[fpgaId:0x%x] Faild!\n",  chip_id);
    }
    
    return nwrite;
}

int m57_unload_bitfile_from_fpga(uint16_t chip_id)
{
    if(io_xdma_is_valid_chipid(chip_id) == false){
        printf_err("0x%x is not valid chipid\n", chip_id);
        return -1;
    }
    return _m57_start_unload_bitfile_from_fpga(chip_id);
}

static int  _unload_bitfile_by_hashid(void *args, int hid, int prio)
{
    uint16_t chip_id;
    chip_id =  GET_SLOT_CHIP_ID_BY_HASHID(hid),
    //printf_note("chip_id:0x%x, hid:0x%x, 0x%x, 0x%x\n", chip_id, hid, GET_SLOTID_BY_HASHID(hid)<<8, GET_CHIPID_BY_HASHID(hid));
    m57_unload_bitfile_from_fpga(chip_id);
    return 0;
}

int m57_unload_bitfile_by_client(struct sockaddr_in *addr)
{
    struct net_tcp_client *cl = container_of(addr, struct net_tcp_client, peer_addr);

    if(cl == NULL || cl->section.hash == NULL)
        return -1;

    if(cl->section.is_unloading == false){
        cl->section.is_unloading = true;
        net_hash_for_each(cl->section.hash, _unload_bitfile_by_hashid, cl->section.hash);
    }
    cl->section.is_unloading = false;

    return 0;
}

static int _unload_bit_callback(struct net_tcp_client *cl, void *args)
{
    m57_stop_load((void *)cl);
    return m57_unload_bitfile_by_client(&cl->peer_addr);
}

int m57_unload_bitfile_all(void)
{
    tcp_client_do_for_each(_unload_bit_callback, NULL, 0, NULL);
    return 0;
}


static void print_array(uint8_t *ptr, ssize_t len)
{
    if(ptr == NULL || len <= 0)
        return;
    
    for(int i = 0; i< len; i++){
        printf("%02x ", *ptr++);
    }
    printf("\n");
}


static int _m57_write_data_to_fpga(uint8_t *ptr, size_t len)
{
    struct spm_context *_ctx;
    ssize_t nwrite;
    void *buffer = NULL;
    int pagesize = 0,reminder = 0, _len = 0;

    pagesize=getpagesize();
    posix_memalign((void **)&buffer, pagesize /*alignment */ , len + pagesize);
    if (!buffer) {
        fprintf(stderr, "OOM %u.\n", pagesize);
        return -1;
    }
    memset(buffer, 0 , len + pagesize);
    memcpy(buffer, ptr, len);
    _ctx = get_spm_ctx();
    _len = len;
    reminder = _align_4byte((int *)&_len);
    nwrite = _ctx->ops->write_xdma_data(0, buffer, _len);
    printfd("Write data %s![%ld],align 4byte reminder=%d[raw len: %lu]\n",  (nwrite == _len) ? "OK" : "Faild", nwrite, reminder, len);
    free(buffer);
    buffer = NULL;
    return nwrite;
}

static void _assamble_resp_payload(struct net_tcp_client *cl, int16_t chipid, int ret)
{
    struct _resp{
        int16_t chipid;
        int32_t ret; 
    }__attribute__ ((packed));
    
    /* auto free data when request down */
    struct _resp *r_resp = safe_malloc(sizeof(struct _resp));
    r_resp->chipid = chipid;
    r_resp->ret = ret;
    cl->response.data = r_resp;
    cl->response.response_length = sizeof(struct _resp);
}

static bool _assamble_keytool_resp(struct net_tcp_client *cl, void *header)
{
    uint8_t *info;
    int nbyte = 0, h_nbyte;
    void*resp;
    uint8_t *ptr;

    struct data_frame_t data_header, *pheader = NULL;
    uint8_t data_ex_header[] = {0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x40, 0x01};
    uint8_t data_end[] = {0xff, 0xff};

    pheader = (struct data_frame_t *)((uint8_t *)header + 2);
    nbyte = io_keytool_read_e2prom_file(pheader->py_dist_addr, (void **)&info);
    if(nbyte <= 0){
        nbyte = _reg_get_board_info(0, (void **)&info);
    }
    h_nbyte = nbyte + sizeof(data_header) + sizeof(data_ex_header) + sizeof(data_end);
    if ((resp = safe_malloc(h_nbyte)) == NULL){
      printf_err("Unable to malloc response buffer  %d bytes!", h_nbyte);
      return false;
    }
    ptr = resp;
    memset(ptr, 0, h_nbyte);
    memset(&data_header, 0, sizeof(data_header));
    
    data_header.py_dist_addr = pheader->py_src_addr;
    data_header.py_src_addr = pheader->py_dist_addr;
    data_header.dist_addr = pheader->dist_addr;
    data_header.type = 0x0202;
    data_header.src_addr = pheader->src_addr;
   // printf_note("data_header.src_addr:0x%x\n", data_header.src_addr);
    data_header.port = 0x01;//pheader->port;
   // printf_note("data_header.port:0x%x\n", data_header.port);
    data_header.len = h_nbyte - sizeof(data_ex_header);
    
    memcpy(ptr, &data_header, sizeof(data_header));
    ptr += sizeof(data_header);
    
    memcpy(ptr, &data_ex_header[0], sizeof(data_ex_header));
    ptr += sizeof(data_ex_header);

    memcpy(ptr, info, nbyte);
    ptr += nbyte;

    memcpy(ptr, data_end, sizeof(data_end));
    //printf_note("====%d, 0x%x\n", h_nbyte, h_nbyte);
    //print_array(resp, h_nbyte);

    cl->response.data = resp;
    cl->response.response_length = h_nbyte;
    cl->response.prio = 1;
    
    safe_free(info);
    return true;
}


static void _assamble_keytool_resp_(struct net_tcp_client *cl, void *header)
{
    uint8_t *info;
    int nbyte = 0, h_nbyte;
    void*resp;
    uint8_t *ptr;

    struct data_frame_t data_header, *pheader = NULL;
    uint8_t data_ex_header[] = {0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x40, 0x01};
    uint8_t data_end[] = {0xff, 0xff};

    nbyte = _reg_get_board_info(0, (void **)&info);
    
    h_nbyte = nbyte + sizeof(data_header) + sizeof(data_ex_header) + sizeof(data_end);
    if ((resp = safe_malloc(h_nbyte)) == NULL){
      printf_err("Unable to malloc response buffer  %d bytes!", h_nbyte);
      return;
    }
    ptr = resp;
    memset(ptr, 0, h_nbyte);
    memset(&data_header, 0, sizeof(data_header));
    pheader = (struct data_frame_t*)((uint8_t *)header + 2);
    data_header.py_dist_addr = pheader->py_src_addr;
    data_header.py_src_addr = pheader->py_dist_addr;
    data_header.dist_addr = pheader->dist_addr;
    data_header.type = 0x0202;
    data_header.src_addr = pheader->src_addr;
   // printf_note("data_header.src_addr:0x%x\n", data_header.src_addr);
    data_header.port = 0x01;//pheader->port;
   // printf_note("data_header.port:0x%x\n", data_header.port);
    data_header.len = h_nbyte - sizeof(data_ex_header);
    
    memcpy(ptr, &data_header, sizeof(data_header));
    ptr += sizeof(data_header);
    
    memcpy(ptr, &data_ex_header[0], sizeof(data_ex_header));
    ptr += sizeof(data_ex_header);

    memcpy(ptr, info, nbyte);
    ptr += nbyte;

    memcpy(ptr, data_end, sizeof(data_end));
    //printf_note("====%d, 0x%x\n", h_nbyte, h_nbyte);
    //print_array(resp, h_nbyte);

    cl->response.data = resp;
    cl->response.response_length = h_nbyte;
    cl->response.prio = 1;
    
    safe_free(info);
    return;
}

static void _write_keytool_handle(struct net_tcp_client *cl, void *header)
{
    struct data_frame_t *pheader = NULL;
    uint8_t *payload;
    char filename[128];
    size_t payload_len = 0;
    pheader = (struct data_frame_t*)((uint8_t *)header + 2);
    payload = (uint8_t *)pheader + sizeof(struct data_frame_t) + 4;
    payload_len = pheader->len - 10;
    snprintf(filename, sizeof(filename) -1, "%x", pheader->py_dist_addr);
    printf_note("filename:%s, py_dist_addr:0x%x, py_src_addr:0x%x, payload_len:0x%lx[%lu]\n",filename, pheader->py_dist_addr, pheader->py_src_addr, payload_len, payload_len);
    io_keytool_write_e2prom_file(filename, payload, payload_len);
    return;
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
    const char *payload = (char *)cl->dispatch.body + ex_header_len;
    int payload_len = cl->request.content_length - ex_header_len;
    int _code = -1;
    
    struct m57_ex_header_cmd_st *header;
    header = (struct m57_ex_header_cmd_st *)cl->dispatch.body;  //扩展头包含在body中
    printf_debug("receive cmd[0x%x], payload_len=%d\n", header->cmd, payload_len);
    if(payload_len < 0){
        printf_err("recv cmd data len err\n");
    }
    
    if (header->cmd != CCT_FILE_TRANSFER) {
        printf_note("[%d]recv cmd:  0x%04x\n", cl->get_peer_port(cl), header->cmd);
    }

    switch (header->cmd)
    {
        case CCT_KEY_READ_E2PROM:
        {
            printf_info("Parse READ Key info CMD\n");
            if(_assamble_keytool_resp(cl,  (void*)header))
                _code = 1;
            break;
        }
        case CCT_KEY_WRITE_E2PROM:
        {
            printf_info("Write Key info CMD\n");
            _write_keytool_handle(cl,  (void*)header);
            break;
        }
        case CCT_BEAT_HART:
        {
            uint16_t beat_count = 0;
            uint16_t beat_status = 0;
            beat_count = *(uint16_t *)payload;
            beat_status = *((uint16_t *)payload + 1);
            printf_debug("keepalive beat_count=%d, beat_status=%d\n", beat_count, beat_status);
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
            int enable = 1;
            m57_prio_type _type;
            memcpy(&_reg, payload, sizeof(_reg));
            _type = (_reg.type == 1 ? M57_PRIO_LOW: 1);
            printf_note("=====>>>%s[%d, %d]mode:%d, port:%d\n", _type == 1 ? "Urgent" : "Normal", _type, _reg.type,_reg.type_mode, cl->get_peer_port(cl));
            cl->section.prio = _type;
            //net_hash_add(cl->section.hash, _type, RT_PRIOID);
            #ifdef DEBUG_TEST
             struct sub_st{
                uint16_t chip_id;
                uint16_t func_id;
                uint16_t port;
            }__attribute__ ((packed));
            struct sub_st _sub;
            _sub.chip_id = 0x0502;
            _sub.func_id = 0;
            _sub.port = 0x0001;
            cl->section.prio = 0;
            net_hash_add_ex(cl->section.hash, GET_HASHMAP_ID(_sub.chip_id, _sub.func_id, cl->section.prio, _sub.port));
            #endif
            for(int ch = 0; ch < MAX_XDMA_NUM; ch++)
                executor_set_command(EX_XDMA_ENABLE_CMD, -1, ch, &enable, -1);
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
            if(io_xdma_is_valid_chipid(_sub.chip_id) == false){
                printf_err("0x%x is not valid chipid\n", _sub.chip_id);
                break;
            }
            printf_note("[%d]sub chip_id:0x%x, func_id=0x%x, port=0x%x, prio=%d\n", cl->get_peer_port(cl), _sub.chip_id, _sub.func_id, _sub.port, cl->section.prio);
            io_socket_set_sub(cl->section.section_id, _sub.chip_id, _sub.func_id, _sub.port);
            net_hash_add_ex(cl->section.hash, GET_HASHMAP_ID(_sub.chip_id, _sub.func_id, cl->section.prio, _sub.port));
            printf_note("[%d,prio:%s]hash id: 0x%x\n", cl->get_peer_port(cl), (cl->section.prio==0?"low":"high"), GET_HASHMAP_ID(_sub.chip_id, _sub.func_id, cl->section.prio, _sub.port));
            /* 
                客户端默认命令都从高优先级socket发送到设备，这里设备程序对高低优先级socket都做了订阅，具体往哪里发，取决读取数据优先级 
                0： 低优先级    1:高优先级
            */
            struct net_tcp_client *cl_prio;
            int _prio = 0;
            if(cl->section.prio == 0){
                _prio = 1;
            }
            cl_prio = tcp_find_prio_client(cl, _prio);
            if(cl_prio != NULL){
                net_hash_add_ex(cl_prio->section.hash, GET_HASHMAP_ID(_sub.chip_id, _sub.func_id, cl_prio->section.prio, _sub.port));
                printf_note("[%d,prio:%s]hash id: 0x%x\n", cl_prio->get_peer_port(cl_prio), (cl_prio->section.prio==0?"low":"high"), GET_HASHMAP_ID(_sub.chip_id, _sub.func_id, cl_prio->section.prio, _sub.port));
            }
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
            printf_note("[%d]unsub chip_id:0x%x, func_id=0x%x, port=0x%x, prio=%d\n", cl->get_peer_port(cl),_sub.chip_id, _sub.func_id, _sub.port, cl->section.prio);
            io_socket_set_unsub(cl->section.section_id, _sub.chip_id, _sub.func_id, _sub.port);
            net_hash_del_ex(cl->section.hash, GET_HASHMAP_ID(_sub.chip_id, _sub.func_id, cl->section.prio, _sub.port));
            break;
        }
        case CCT_LIST_INFO:
        {
            uint8_t *info;
            int nbyte = 0;
            #ifdef DEBUG_TEST
            nbyte = _reg_get_fpga_info(NULL, 0, (void **)&info);
            #else
            nbyte = _reg_get_fpga_info_(get_fpga_reg(), 0, (void **)&info);
            #endif
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
            if(io_xdma_is_valid_chipid(id) == false){
                printf_err("0x%x is not valid chipid\n", id);
                _assamble_resp_payload(cl, id, -1);
                _code = CCT_RSP_UNLOAD;
                break;
            }
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
            if(io_xdma_is_valid_chipid(pload->chipid) == false){
                printf_err("0x%x is not valid chipid\n", pload->chipid);
                _assamble_resp_payload(cl, pload->chipid, -1);
                _code = CCT_RSP_LOAD;
                break;
            }
            //拨码开关地址检测
            #if 0
            if(io_xdma_is_valid_addr(CARD_SLOT_NUM(cl->section.chip_id))){
                printf_err("maybe Dial switch addr not correct\n", pload->chipid);
                _assamble_resp_payload(cl, pload->chipid, -1);
                _code = CCT_RSP_LOAD;
                break;
            }
            #endif
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
                goto load_file_exit;
                //break;
            }
            pinfo = (struct file_xinfo *)payload;
            if(pinfo->sn == 0)
                printf_note("start transfer file\n");
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
                ret = M57_CARD_STATUS_LOAD_FAILD;
                if(cl->section.is_loadfile_ok && (io_xdma_is_valid_chipid(cl->section.chip_id) == true)){
#if LOAD_FILE_ASYN
                    //异步加载
                    if(m57_loading_bitfile_to_fpga_asyn(cl, cl->section.file.path) == 0){
                        ret = M57_CARD_STATUS_OK;
                    }
#else
                    if(_m57_loading_bitfile_to_fpga(cl->section.file.path)){
                        _m57_load_bit_over(cl, true);
                        ret = M57_CARD_STATUS_OK;
                    }else{
                        _m57_load_bit_over(cl, false);
                    }
#endif
                }
                if(ret != M57_CARD_STATUS_OK){
                    ns_downlink_set_loadbit_result(CARD_SLOT_NUM(cl->section.chip_id), DEVICE_STATUS_LOAD_ERR);
                    config_set_device_status(DEVICE_STATUS_LOAD_ERR, cl->section.chip_id);
                    _assamble_resp_payload(cl, cl->section.chip_id, ret);
                    _code = CCT_RSP_LOAD;
                }
            }else{
                //set loading status
                ns_downlink_set_loadbit_result(CARD_SLOT_NUM(cl->section.chip_id), DEVICE_STATUS_LOADING);
                config_set_device_status(DEVICE_STATUS_LOADING, cl->section.chip_id);
            }
            break;
        }
    }
    if(code >= 0)
        cl->response.need_resp = true;
    *code = _code;
    return true;
}

bool m57_execute(void *client, int *code)
{
    struct net_tcp_client *cl = client;

    //update_tcp_keepalive(cl);
    //if(cl->request.data_type == M57_DATA_TYPE)
    //    return m57_execute_data(client, code);
    //else 
    if(cl->request.data_type == M57_CMD_TYPE)
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
    memset(&ex_header, 0, sizeof(ex_header));
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
    _safe_free_(psend);
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
    //cl->section.prio = M57_PRIO_LOW;
    printf_debug("send beatheart to:[%s:%d],n=%d \n", cl->get_peer_addr(cl), cl->get_peer_port(cl), beatheart.beat_count);
    m57_send_cmd(client, CCT_BEAT_HART, &beatheart, sizeof(beatheart));
}

void m57_send_resp(void *client, int code, void *args)
{
    struct net_tcp_client *cl = client;
    static uint8_t sn = 0;
    struct m57_header_st header;
    void *ex_header = NULL;
    int type;
    int hlen = sizeof(struct m57_header_st);
    struct m57_ex_header_cmd_st _ex_header;
    int exhlen = 0;
    uint8_t *psend;
    
    if(code <= 0)
        return;

    printf_debug("code = [0x%x, %d], keytool:%d\n", code, code, cl->section.is_keytool_cmd);
    memset(&header, 0, sizeof(header));
    header.header = M57_SYNC_HEADER;
    if(cl->section.is_keytool_cmd){
        header.type = M57_DATA_TYPE;
        cl->section.is_keytool_cmd = false;
    }
    else
        header.type = M57_CMD_TYPE;
    header.prio = cl->response.prio;
    header.number = sn++;

    if(header.type == M57_DATA_TYPE){
        exhlen = 0;
    } else {
        exhlen = sizeof(struct m57_ex_header_cmd_st);
        memset(&_ex_header, 0, sizeof(_ex_header));
        _ex_header.cmd = code;
        _ex_header.len = exhlen + cl->response.response_length;
        ex_header = &_ex_header;
    }
    _m57_swap_send_header_element(&header);

    header.len = hlen  + exhlen + cl->response.response_length;
    psend = calloc(1, header.len);
    if (!psend) {
            printf_err("calloc\n");
            return;
        }
    memcpy(psend, &header, hlen);
    if(exhlen > 0 && ex_header)
        memcpy(psend + hlen, ex_header, exhlen);

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

