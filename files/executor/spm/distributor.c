/******************************************************************************
*  Copyright 2021, Showay Technology Dev Co.,Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Showay Technology Dev Co.,Ltd. (C) 2019
******************************************************************************/
/*****************************************************************************     
*  Rev 1.0   1 May. 2021   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"
#include "distributor.h"
#include "spm_fpga.h"


struct hash_type_table htable[] = {
    {0, {DTR_STREAM_TYPE_AUDIO}, NULL, "audio"},
    {1, {DTR_STREAM_TYPE_NIQ},   NULL, "niq"},
    {2, {DTR_STREAM_TYPE_BIQ},   NULL, "biq"},
    {3, {DTR_STREAM_TYPE_FFT},   NULL, "fft"},
};

void **_distributor_init(int index_num)
{
    struct distributor **dtr;
    dtr =  (struct distributor **)calloc(1, index_num);
    for(int i = 0; i< index_num; i++){
        dtr[i] =  (struct distributor *)calloc(1, sizeof(struct distributor));
        if (!dtr[i]) {
            printf_err("calloc\n");
            return NULL;
        }
        INIT_LIST_HEAD(&dtr[i]->lists);
    }
    return (void **)dtr;
}

void **distributor_init(void)
{
    return _distributor_init(DTR_STREAM_TYPE_MAX);
}


void distributor_free(void *list)
{
    struct dtr_data_node *nlist, *list_tmp;
    struct distributor *dtr = (struct distributor *)list;
    list_for_each_entry_safe(nlist, list_tmp, &dtr->lists, list){
        list_del(&nlist->list);
        free(nlist);
        nlist = NULL;
    }
}


int distributor_node_add(int index, void **list, void *data, size_t len)
{
    struct dtr_data_node *node;
    struct distributor **dtr = (struct distributor **)list;
    
    node = calloc(1, sizeof(struct dtr_data_node));
    if (!node) {
        printf_err("calloc\n");
        return -1;
    }
    node->data = data;
    node->len = len;
    list_add_tail(&node->list, &dtr[index]->lists);
    return 0;
}

void *distributor_node_get(int index,  void **list)
{
    struct distributor **dtr = (struct distributor **)list;
    
    if(index >= DTR_STREAM_TYPE_MAX)
        return NULL;
        
    return dtr[index];
}


int distributor_node_remove(void *node)
{
    struct dtr_data_node *_node = node;
    list_del(&_node->list);
    return 0;
}

int distributor_node_dump(void *list)
{
    struct dtr_data_node *nlist, *list_tmp;
    struct distributor *dtr = (struct distributor *)list;
    list_for_each_entry_safe(nlist, list_tmp, &dtr->lists, list){
        printf("data=%s, len=%ld\n", (char *)nlist->data, nlist->len);
    }
    return 0;
}

int distributor_node_dump_all(void **list, size_t num)
{
    struct dtr_data_node *nlist, *list_tmp;
    struct distributor **dtr = (struct distributor **)list;
    for(int i = 0; i < num; i++){
        distributor_node_dump(dtr[i]);
    }

    return 0;
}

#if 0
int distributor_iq_send(void *args, ssize_t len)
{
    struct spm_run_parm *ptr_run = args;
    struct distributor **dtr = (struct distributor **)ptr_run->distributor;
    struct dtr_data_node *nlist, *list_tmp;
    uint32_t i = 0, j =0;
    
    /*send audio data*/
    i = 0;
    if (!list_empty(&dtr[DTR_STREAM_TYPE_AUDIO]->lists)){
        list_for_each_entry_safe(nlist, list_tmp, &dtr[DTR_STREAM_TYPE_AUDIO]->lists, list){
            //printf_note("send audio= %p, 0x%x, len=%ld, %u\n", nlist->data, *(iq_t *)nlist->data, nlist->len, i++);
            i++;
            spm_send_audio_data(nlist->data, nlist->len, args);
        }
        distributor_free((void *)dtr[DTR_STREAM_TYPE_AUDIO]);
    }else{
        //printf_note("audio empty\n");
    }
    
   // printf_note("audio list empty %d\n", list_empty(&dtr[DTR_STREAM_TYPE_AUDIO]->lists));

    /*send niq data*/
    j = 0;
    if (!list_empty(&dtr[DTR_STREAM_TYPE_NIQ]->lists)){
        list_for_each_entry_safe(nlist, list_tmp, &dtr[DTR_STREAM_TYPE_NIQ]->lists, list){
            //printf_note("send niq= %p, 0x%x, len=%ld, %u\n", nlist->data, *(iq_t *)nlist->data, nlist->len, i++);
            j++;
            spm_send_iq_data(nlist->data, nlist->len, args);
        }
        distributor_free((void *)dtr[DTR_STREAM_TYPE_NIQ]);
    }else{
        //printf_note("niq empty\n");
    }
    //spm_read_iq_over_deal(&len);
    //printf_note("nio list empty %d\n", list_empty(&dtr[DTR_STREAM_TYPE_NIQ]->lists));
    
    //static int k = 0;
    //printf_note("------------------[%d]send count:audio:%d, niq:%d--------------------\n", k, i, j);
    //if(k++ > 15)
    //    exit(0);
    
    return 0;
}

/* 将不同IQ通道数据，按通道归类添加到链表 */
int distributor_iq(void *iq_data, size_t len, void *args)
{
    #define AUDIO_CHANNEL CONFIG_AUDIO_CHANNEL
    #define IQ_DATA_PACKAGE_BYTE 512
    #define IQ_HEADER_1 0x55aa
    #define IQ_HEADER_2 0xe700

    struct spm_run_parm *ptr_run = args;
    struct distributor **dtr = (struct distributor **)ptr_run->distributor;

    iq_t *ptr_iq = (iq_t *)iq_data;
    iq_t *iq_head = ptr_iq;
    int subch = -1;
    size_t offset = IQ_DATA_PACKAGE_BYTE/sizeof(iq_t);
    size_t niq_count = 0, audio_count = 0, niq_num = 0, audio_num = 0;
    iq_t *niq_cur = ptr_iq;
    iq_t *audio_cur = ptr_iq;

   // printf_note("len=%ld\n", len);
    for(int i = 0; i< len; i += IQ_DATA_PACKAGE_BYTE){
        if(((*iq_head & 0x0ff00) == IQ_HEADER_2) && (*(iq_head+1) == IQ_HEADER_1)){
            subch = *iq_head & 0x00ff;
            if(subch == AUDIO_CHANNEL){
                if(audio_count == 0){
                    audio_cur = iq_head;
                }
                audio_count ++;
                audio_num++;
                if(niq_count != 0){
                    //printf_note("push niq list: %p, 0x%x, len=%ld\n", niq_cur, *(iq_t *)niq_cur, IQ_DATA_PACKAGE_BYTE*niq_count);
                    distributor_node_add(DTR_STREAM_TYPE_NIQ, (void **)dtr, (void *)niq_cur, IQ_DATA_PACKAGE_BYTE*niq_count);
                    niq_count = 0;
                }
            }
            else{
                if(niq_count == 0){
                    niq_cur = iq_head;
                }
                niq_count++;
                niq_num++;
                if(audio_count != 0){
                    //printf_note("push audio list:%p, 0x%x, len=%ld\n", audio_cur, *(iq_t *)audio_cur, IQ_DATA_PACKAGE_BYTE*audio_count);
                    distributor_node_add(DTR_STREAM_TYPE_AUDIO, (void **)dtr, (void *)audio_cur, IQ_DATA_PACKAGE_BYTE*audio_count);
                    audio_count = 0;
                }
            }
        }
        iq_head += offset;
    }
    //printf_note("[%ld]niq_num=%ld, audio_num = %ld, niq_count=%ld, audio_count=%ld, ptr_iq=0x%x\n", 
    //             len, niq_num, audio_num, niq_count, audio_count, *ptr_iq);
    if(niq_count != 0){
        //printf_note("push niq list: %p, 0x%x, len=%ld\n",niq_cur,  *(iq_t *)niq_cur, IQ_DATA_PACKAGE_BYTE*niq_count);
        distributor_node_add(DTR_STREAM_TYPE_NIQ, (void **)dtr, (void *)niq_cur, IQ_DATA_PACKAGE_BYTE*niq_count);
        niq_count = 0;
    }
    if(audio_count != 0){
        //printf_note("push audio list: %p, 0x%x, len=%ld\n", audio_cur, *(iq_t *)audio_cur, IQ_DATA_PACKAGE_BYTE*audio_count);
        distributor_node_add(DTR_STREAM_TYPE_AUDIO, (void **)dtr, (void *)audio_cur, IQ_DATA_PACKAGE_BYTE*audio_count);
        audio_count = 0;
    }
    return 0;
}

int distributor_iq_(void *iq_data, size_t len, void *args)
{
    #define AUDIO_CHANNEL CONFIG_AUDIO_CHANNEL
    #define IQ_DATA_PACKAGE_BYTE 512
    #define IQ_HEADER_1 0x55aa
    #define IQ_HEADER_2 0xe700

    struct spm_run_parm *ptr_run = args;
    struct distributor **dtr = (struct distributor **)ptr_run->distributor;

    iq_t *ptr_iq = (iq_t *)iq_data;
    iq_t *iq_head = ptr_iq;
    int subch = -1;
    size_t offset = IQ_DATA_PACKAGE_BYTE/sizeof(iq_t);
    size_t niq_count = 0, audio_count = 0;
    iq_t *niq_cur = ptr_iq;
    iq_t *audio_cur = ptr_iq;

    for(int i = 0; i< len; i += IQ_DATA_PACKAGE_BYTE){
        if(((*iq_head & 0x0ff00) == IQ_HEADER_2) && (*(iq_head+1) == IQ_HEADER_1)){
            subch = *iq_head & 0x00ff;
            if(subch == AUDIO_CHANNEL){
                distributor_node_add(DTR_STREAM_TYPE_AUDIO, (void **)dtr, (void *)iq_head, IQ_DATA_PACKAGE_BYTE);
            }
            else{
                distributor_node_add(DTR_STREAM_TYPE_NIQ, (void **)dtr, (void *)iq_head, IQ_DATA_PACKAGE_BYTE);
            }
        }
        iq_head += offset;
    }
    return 0;
}

#endif


int distributor_main(void)
{
    struct distributor **ptr;
    ptr = (struct distributor **)distributor_init();
    distributor_node_add(0, (void **)ptr, "ch0 test1", 5);
    distributor_node_add(0, (void **)ptr, "ch0 test2", 6);
    distributor_node_add(0, (void **)ptr, "ch0 test3....", 8);
    distributor_node_add(1, (void **)ptr, "test2", 6);
    distributor_node_add(2, (void **)ptr, "test3", 7);
    distributor_node_add(3, (void **)ptr, "test4", 8);
    distributor_node_dump_all((void **)ptr, 4);
    //distributor_node_dump(ptr[0]);
    //distributor_node_dump(ptr[1]);
    //distributor_node_dump(ptr[2]);
    distributor_free(ptr[0]);
    distributor_free(ptr[1]);
    return 0;
}
