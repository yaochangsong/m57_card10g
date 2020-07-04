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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <fcntl.h>              // Flags for open()
#include <sys/stat.h>           // Open() system call
#include <sys/types.h>          // Types for open()
#include <unistd.h>             // Close() system call
#include <string.h>             // Memory setting and copying
#include <getopt.h>             // Option parsing
#include <errno.h>              // Error codes
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>


#define SERVER_IP "192.168.3.11"
#define SERVER_PORT 8888

#define MAX_THREAD_NUM 8



struct net_info {
    int fd;          // The file descriptor for the output file
    int buffer_size;
    int packet_count;
    struct sockaddr_in dest_addr;  //The destnation socket address
};

struct thread_pool
{
	int thread_num;
    pthread_t thread_ids[MAX_THREAD_NUM];
    pthread_attr_t thread_attr[MAX_THREAD_NUM];
	struct net_info eInfo;
};

void usage(char *prog)
{
	printf("usage: %s\n",prog);
	printf("%s -t [thread numbers <8] -s [buffer size]\n", prog);
}


static uint32_t getopt_integer(char *optarg)
{
  int rc;
  uint32_t value;
  rc = sscanf(optarg, "0x%x", &value);
  if (rc <= 0)
    rc = sscanf(optarg, "%ul", &value);
    
  return value;
}

int init_net(struct net_info *net, int size)
{
	
	int client_fd = -1;
	int len_buf = 0;
	struct sockaddr_in ser_addr;
	
	client_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(client_fd < 0)
     {
         printf("create socket fail!\n");
         return -1;
     }
 	net->fd = client_fd;
 	//net->buffer_size = 65507;
	net->buffer_size = size; //max value can be set to 65507
 	net->packet_count = 1000000;
	memset(&net->dest_addr, 0, sizeof(net->dest_addr));
	net->dest_addr.sin_family = AF_INET;
	net->dest_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	//net->dest_addr.sin_addr.s_addr = inet_addr(ipdaar);
	net->dest_addr.sin_port = htons(SERVER_PORT); 	
	//getsockopt(client_fd, SOL_SOCKET, SO_SNDBUF, &len_buf, sizeof(len_buf));
	//printf("socket send buf len:%d\n", len_buf);
	//setsockopt(client_fd, SOL_SOCKET, SNDBUF, );
	return 0;
}

 static int net_send(struct net_info *net)
{
	 int rc;
 	 int i;
	 void *output_buf;
	 int pagesize;
	 
	 pagesize=getpagesize();
	 posix_memalign((void **)&output_buf, pagesize /*alignment */ , net->buffer_size + pagesize);
	 // Allocate a buffer for the output file
	// output_buf = malloc(net->buffer_size);
	 if (output_buf == NULL) {
		 rc = -ENOMEM;
		 goto ret;
	 }

	 memset(output_buf, 0xaa, net->buffer_size);

	 printf("start net send\n");
 	for(i = 0; i < net->packet_count; i++)
 	{
 		rc = sendto(net->fd, output_buf, net->buffer_size, 0, &net->dest_addr, sizeof(struct sockaddr_in));
    	if(rc <= 0)
    		perror("send to ");
	}
	
	free(output_buf);
 ret:
	 return rc;	
}

static void thread_start(void *arg)
{
	struct net_info *trans = arg;
	printf("thread_start\n");
    net_send(trans);
	printf("thread_end\n");
	return;
}

int create_thread_pool(struct thread_pool *thread_pool, int num, struct net_info *net)
{
    pthread_t thread_id;
    pthread_attr_t thread_attr;
	int i, ret;
	struct sched_param param;
	
	for(i = 0; i < num; i++)
	{
		pthread_attr_init(&thread_attr);
#if 0
		param.sched_priority = sched_get_priority_min(SCHED_FIFO);
		printf("sched_priority:%d\n", param.sched_priority);
		pthread_attr_setschedpolicy(&thread_attr, SCHED_FIFO);
		pthread_attr_setschedparam(&thread_attr, &param);
#endif		
		if (0 != pthread_create(&thread_id, &thread_attr, thread_start, &net[i]))
		{
		   perror("create thread error\n");
		   return -1;
		}

		/* Destroy the thread attributes object, since it is no
					  longer needed */
		ret = pthread_attr_destroy(&thread_attr);
		if (ret != 0)
		{
			perror("pthread_attr_destroy");
			return -1;
		}
#if 1	
		cpu_set_t cpu_info;
		int cpu_num = i % 4;
		CPU_ZERO(&cpu_info);
		CPU_SET(cpu_num, &cpu_info);
		printf("thread %d on cpu %d\n", i, cpu_num);	
		if (0!=pthread_setaffinity_np(thread_id, sizeof(cpu_set_t), &cpu_info))
		{
		   printf("set affinity failed");
		}
#endif	
		thread_pool->thread_ids[i] = thread_id;
		//thread_pool->thread_attr[i] = thread_attr;
		//pthread_join(thread_id, NULL);
	}
	thread_pool->thread_num = num;
	
	return 0;
}


void start_thread_pool(struct thread_pool *thread_pool)
{
	int i;
	
	for(i = 0; i < thread_pool->thread_num; i++)
	{
		pthread_join(thread_pool->thread_ids[i], NULL);
	}
}



int main(int argc, char **argv)
{
	struct net_info net[MAX_THREAD_NUM];
	struct thread_pool tPool;
	int flag;
	int thread_nums, buf_size, cmd_opt;
	//char *server_ip=NULL;
	
	while ((cmd_opt = getopt_long(argc, argv, "t:s:", NULL, NULL)) != -1)
	{
		switch (cmd_opt)
		{ 
		  case 't':
		    thread_nums = getopt_integer(optarg);
			printf("thread num:%d\n",thread_nums);
		    break;
		  case 's':
		    buf_size = getopt_integer(optarg);
		    break;
		  default:
		  	usage(argv[0]);
		    exit(0);
		    break;
		}
	}

	memset(&net, 0, sizeof(net));
	for(flag=0; flag<thread_nums; flag++)
	{
		init_net(&net[flag], buf_size);
	}
	
	printf("init net ok\n");
	
	create_thread_pool(&tPool, thread_nums, net);

	start_thread_pool(&tPool);
	
#if 1
	for(flag=0; flag<thread_nums; flag++)
	{
		close(net[flag].fd);
	}
#endif	
	printf("net send done.\n");
    return 0;
}

