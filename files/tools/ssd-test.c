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
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <math.h>
#include <inttypes.h>


void usage(char *prog)
{
	printf("usage: %s\n",prog);
	printf("%s -w -f filename -s [block size] -c [block count]\n", prog);
	printf("%s -r -f filename -s [block size]\n", prog);
}

static int timespec_check(struct timespec *t)
{
	if ((t->tv_nsec < 0) || (t->tv_nsec >= 1000000000))
		return -1;
	return 0;
}

static void timespec_sub(struct timespec *t1, struct timespec *t2)
{
	if (timespec_check(t1) < 0) {
		fprintf(stderr, "invalid time #1: %lld.%.9ld.\n",
			(long long)t1->tv_sec, t1->tv_nsec);
		return;
	}
	if (timespec_check(t2) < 0) {
		fprintf(stderr, "invalid time #2: %lld.%.9ld.\n",
			(long long)t2->tv_sec, t2->tv_nsec);
		return;
	}
	t1->tv_sec -= t2->tv_sec;
	t1->tv_nsec -= t2->tv_nsec;
	if (t1->tv_nsec >= 1000000000) {
		t1->tv_sec++;
		t1->tv_nsec -= 1000000000;
	} else if (t1->tv_nsec < 0) {
		t1->tv_sec--;
		t1->tv_nsec += 1000000000;
	}
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

int test_write_disk(char *filename, int size, int count)
{
	void *user_mem = NULL;
	int pagesize = 0;
	int rc;
	struct timespec ts_start, ts_end;
	float speed = 0;
	uint64_t total_MB = 0;
	
	total_MB = (size / (1024 * 1024) * count);

	printf("test write: block size=%d, count=%d, total=%ldMB\n", size, count, total_MB);
	pagesize=getpagesize();
	posix_memalign((void **)&user_mem, pagesize /*alignment */ , size + pagesize);
	if (!user_mem) {
		fprintf(stderr, "OOM %u.\n", pagesize);
		return -1;
	}

	int out_fd = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_DIRECT | O_SYNC, 0666);
	if (out_fd < 0) {
		printf("open %s fail\n", filename);
		exit(1);
	}

	rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);

	while (count--)
	{
		rc = write(out_fd, user_mem, size);
		if (rc < 0)
		{
			//printf("write buffer offset:%u, index:%d\n", offset, index);
			//printf("%s, Write 0x%lx != 0x%lx.\n", filename, rc, ring_trans.block_size);
			perror("write file");
			goto done;
		}
		else if(rc != size)
		{
			printf("%s, Write 0x%x != 0x%x.\n", filename, rc, size);
		}
	}
	sync();
	clock_gettime(CLOCK_MONOTONIC, &ts_end);
	timespec_sub(&ts_end, &ts_start);
	float total_t = ts_end.tv_sec + (float)(ts_end.tv_nsec / 1000000000.0);
	fprintf(stdout,"total time: %.5f sec\n", total_t);
	speed = (float)(total_MB / total_t);
	fprintf(stdout,"speed: %.2f MBps\n", speed);
done:	
	free(user_mem);
	close(out_fd);
	return 0;
}

int test_read_disk(char *filename, int size)
{
	void *user_mem = NULL;
	int pagesize = 0;
	int rc;
	struct timespec ts_start, ts_end;
	float speed = 0;
	uint64_t total_MB = 0;
    struct stat input_stat;
    int count = 0;
    
	pagesize=getpagesize();
	posix_memalign((void **)&user_mem, pagesize /*alignment */ , size + pagesize);
	if (!user_mem) {
		fprintf(stderr, "OOM %u.\n", pagesize);
		return -1;
	}

	int in_fd = open(filename, O_RDONLY | O_DIRECT | O_SYNC, 0666);
	if (in_fd < 0) {
		printf("open %s fail\n", filename);
		exit(1);
	}

    if (fstat(in_fd, &input_stat) < 0) {
        perror("Unable to get file statistics");
        rc = 1;
        goto done;
    }

	count = input_stat.st_size / size;
    total_MB = input_stat.st_size / (1024 * 1024);
	printf("test read: block size=%d, count=%d, total=%"PRIu64" MB\n", size, count, total_MB);

	rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);
	while (count--)
	{
		rc = read(in_fd, user_mem, size);
		if (rc < 0)
		{
			//printf("write buffer offset:%u, index:%d\n", offset, index);
			//printf("%s, Write 0x%lx != 0x%lx.\n", filename, rc, ring_trans.block_size);
			perror("read file");
			goto done;
		}
		else if(rc != size)
		{
			printf("%s, Write 0x%x != 0x%x.\n", filename, rc, size);
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &ts_end);
	timespec_sub(&ts_end, &ts_start);
	float total_t = ts_end.tv_sec + (float)(ts_end.tv_nsec / 1000000000.0);
	fprintf(stdout,"total time: %.5f sec\n", total_t);
	speed = (float)(total_MB / total_t);
	fprintf(stdout,"speed: %.2f MBps\n", speed);
	
done:	
	free(user_mem);
	close(in_fd);
	return 0;
}

int test_write_mem(int size, int count)
{
	void *user_mem_src = NULL;
	void *user_mem_dst = NULL;
	int pagesize = 0;
	int rc;
	struct timespec ts_start, ts_end;
	
	pagesize=getpagesize();
	posix_memalign((void **)&user_mem_src, pagesize /*alignment */ , size + pagesize);
	if (!user_mem_src) {
		fprintf(stderr, "OOM %u.\n", pagesize);
		return -1;
	}

	posix_memalign((void **)&user_mem_dst, pagesize /*alignment */ , size + pagesize);
	if (!user_mem_dst) {
		fprintf(stderr, "OOM %u.\n", pagesize);
		return -1;
	}
	
	rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);

	while (count--)
	{
		memcpy(user_mem_dst, user_mem_src, size);
	}
	
	clock_gettime(CLOCK_MONOTONIC, &ts_end);
	timespec_sub(&ts_end, &ts_start);
	fprintf(stdout,"CLOCK_MONOTONIC %ld.%09ld sec. write %d bytes\n", ts_end.tv_sec, ts_end.tv_nsec, size * count);
	return 0;
}

int main(int argc, char **argv)
{
	int cmd_opt;
	uint32_t block_size = 8388608;  //default 8M
	uint32_t count = 1;
	char *filename = NULL;
	//int ret  = 0;
	//int memtest = 0;
	int rw = 0;
	
	while ((cmd_opt = getopt_long(argc, argv, "rwc:s:f:", NULL, NULL)) != -1)
	{
		switch (cmd_opt)
		{ 
		  case 'r':
		    rw = 1;
		    break;
		  case 'w':
		    rw = 0;
		    break;			
		 case 'f':   
		    filename = strdup(optarg);
		    break;
		  case 's':
		    block_size = getopt_integer(optarg);
		    break;     
		  case 'c':
		    count = getopt_integer(optarg);
		    break;
		  default:
		  	usage(argv[0]);
		    exit(0);
		    break;
		}
	}

	if(!filename)
	{
		usage(argv[0]);
		return -1;
	}
	
	if(rw)
	{
		test_read_disk(filename, block_size);
	}
	else
	{
		if(!count)
		{
			usage(argv[0]);
			return -1;
		}
		test_write_disk(filename, block_size, count);
	}
	
    return 0;
}
