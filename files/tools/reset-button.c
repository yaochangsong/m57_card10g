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
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <linux/input.h>
#include <unistd.h>
#include <semaphore.h>

#define    INPUT_EVENT    "/dev/input/event0"

pthread_mutex_t lock;
char cmd[128];
sem_t cmd_sem;

/* handle cmd */
static void *handle_cmd(void *dummy)
{
    while (1) {
        sem_wait(&cmd_sem);
        pthread_mutex_lock(&lock);
        if(strlen(cmd) >0){
            system(cmd);
            memset(cmd,0,sizeof(cmd));
        }
        pthread_mutex_unlock(&lock);
    }
	return NULL;
}

int main()
{
    pthread_t pth;
    struct input_event ev;
    int tmp;
    //int key_code;
    int size = sizeof(ev);
    struct timeval press_time;
    int timediff_sec=0;
    daemon(0,0);
    /* Create thread */
    sem_init(&cmd_sem, 0, 0);
    pthread_mutex_init(&lock, NULL);
    pthread_create(&pth, NULL, handle_cmd, "event handle");
    /* Read event0 */
    tmp = open(INPUT_EVENT, O_RDONLY);
    if (tmp < 0) {
        printf("\nOpen " INPUT_EVENT " failed!\n");
        return 1;
    }
    /* Read and parse event, update global variable */
    while (1) {
        if (read(tmp, &ev, size) < size) {
            //printf("\nReading from " INPUT_EVENT " failed!\n");
            usleep(1000);
            continue;
        }
        //printf("event type:%d,value%d,code=%d\n",ev.type,ev.value,ev.code);
        if(ev.type == EV_KEY){
            //key_event
            if(ev.code == KEY_RESTART){
                //care restart input
                if(ev.value == 1){
                    //press button
                    press_time = ev.time;
                }else if(ev.value == 0){
                    //release button
                    timediff_sec = ev.time.tv_sec - press_time.tv_sec;
                    if(timediff_sec >=5){
                        //reset config
                        pthread_mutex_lock(&lock);
                        sprintf(cmd,"/etc/reset-event.sh %s","reset_config");      
                        pthread_mutex_unlock(&lock);

                        sem_post(&cmd_sem);                    
                    }
                }      
            }
        }
    }
}
