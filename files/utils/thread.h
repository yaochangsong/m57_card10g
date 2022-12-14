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
*  Rev 1.0   22 June 2020   yaochangsong
*  Initial revision.
******************************************************************************/
#ifndef _THREAD_H_
#define _THREAD_H_

extern void pthread_bmp_init(void);
extern int pthread_create_detach (const pthread_attr_t *attr, int (*init_callback) (void *),
                                        int (*start_routine) (void *), int (*exit_callback) (void *), 
                                        char *name, void *arg_cb, void *arg_exit, pthread_t *tid);

extern void *pthread_cancel_by_name(char *name);
extern int pthread_exit_by_name(char *name);
extern bool pthread_check_alive_by_name(char *name);
extern bool pthread_check_alive_by_tid(pthread_t tid);
extern int pthread_cancel_by_tid(pthread_t tid);

#endif
