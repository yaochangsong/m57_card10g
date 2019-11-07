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
*  Rev 1.0   09 July 2019   yaochangsong
*  Initial revision.
******************************************************************************/
#include "config.h"

/** Duplicates a string or die if memory cannot be allocated
 * @param s String to duplicate
 * @return A string in a newly allocated chunk of heap.
 */
char *safe_strdup(const char *s)
{
    char *retval = NULL;
    if (!s) {
        printf_err("safe_strdup called with NULL which would have crashed strdup. Bailing out");
        exit(1);
    }
    retval = strdup(s);
    if (!retval) {
        printf_err("Failed to duplicate a string: %s.  Bailing out", strerror(errno));
        exit(1);
    }
    return (retval);
}


int get_mac(char * mac, int len_limit)    
{
    struct ifreq ifreq;
    int sock;

    if(mac ==NULL)
        return -1;
    
    if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror ("socket");
        return -1;
    }
    strcpy (ifreq.ifr_name, NETWORK_EHTHERNET_POINT);    

    if (ioctl (sock, SIOCGIFHWADDR, &ifreq) < 0)
    {
        perror ("ioctl");
        return -1;
    }
    
    memcpy(mac, &ifreq.ifr_hwaddr.sa_data, len_limit);
    
    return 0;
}


#define CRC_16_POLYNOMIALS   0x8005
uint16_t crc16_caculate(uint8_t *pchMsg, uint16_t wDataLen) {
    uint8_t i;
    uint8_t chChar;
    uint16_t wCRC = 0xFFFF; //g_CRC_value;

    while (wDataLen--) {
        chChar = *pchMsg++;
        wCRC ^= (((uint16_t) chChar) << 8);

        for (i = 0; i < 8; i++) {
            if (wCRC & 0x8000) {
                wCRC = (wCRC << 1) ^ CRC_16_POLYNOMIALS;
            } else {
                wCRC <<= 1;
            }
        }
    }

    return wCRC;
}

int write_file_in_int16(void *pdata, unsigned int data_len, char *filename)
{
    
    FILE *file;
    int16_t *pdata_offset;

    pdata_offset = (int16_t *)pdata;

    file = fopen(filename, "w+b");
    if(!file){
        printf("Open file error!\n");
        return -1;
    }

    fwrite((void *)pdata_offset,sizeof(int16_t),data_len,file);

    fclose(file);

    return 0;
}

int write_file_in_float(void *pdata, unsigned int data_len, char *filename)
{
    
    FILE *file;
    float *pdata_offset;

    pdata_offset = (float *)pdata;

    file = fopen(filename, "w+b");
    if(!file){
        printf("Open file error!\n");
        return -1;
    }

    fwrite((void *)pdata_offset,sizeof(float),data_len,file);

    fclose(file);

    return 0;
}


int read_file(void *pdata, unsigned int data_len, char *filename)
{
        
    FILE *file;
    unsigned int *pdata_offset;

    if(pdata == NULL){
        return -1;
    }

    file = fopen(filename, "r");
    if(!file){
        printf("Open file error!\n");
        return -1;
    }

    fread(pdata,1,data_len,file);

    fclose(file);

    return 0;
}

void* safe_malloc(size_t size) {

    void* result = malloc(size);
    if (result == NULL) {
        fprintf(stderr, "safe_malloc: Memory full. Couldn't allocate %lu bytes.\n",
                (unsigned long)size);
        exit(EXIT_FAILURE);
    }
    /* This is all the debug-help we can do easily */
    for (size_t i = 0; i < size; ++i)
        ((char*)result)[i] = 0;
    
    return result;
}

void* safe_calloc(size_t n, size_t size) {
    void* result = calloc(n, size);
    if (result == NULL) {
        fprintf(stderr, "safe_calloc: Memory full. Couldn't allocate %lu bytes.\n",
                (unsigned long)(n * size));
    }
    return result;
}

void safe_free(void *p)
{
    if (p) {
        free(p);
        p = NULL;
    }
}

int32_t inline diff_time(void)
{
    static struct timeval oldTime; 
    struct timeval newTime; 
    int32_t _t_ms, ntime_ms; 
    
    gettimeofday(&newTime, NULL);
    _t_ms = (newTime.tv_sec - oldTime.tv_sec)*1000 + (newTime.tv_usec - oldTime.tv_usec)/1000; 
    printf_debug("_t_ms=%d ms!\n", _t_ms);
    if(_t_ms > 0)
        ntime_ms = _t_ms; 
    else 
        ntime_ms = 0; 
    memcpy(&oldTime, &newTime, sizeof(struct timeval)); 
    printf_debug("Diff time = %u ms!\n", ntime_ms); 
    return ntime_ms;
}


/* system() 存在父进程内存大的话，存在空间申请失败的问题，这里改写system() */
static int _system(const char * cmdstring)  
{  
    pid_t pid;  
    int status;  
  
    if(cmdstring == NULL)  
    {  
        return (1); //如果cmdstring为空，返回非零值，一般为1  
    }  
      
    if((pid = vfork())<0)  /* system() 这里采用fork()调用，如果父进程内存大的话，存在空间申请失败的问题 */
    {  
        status = -1; //fork失败，返回-1  
    }  
    else if(pid == 0)  
    {  
        execl("/bin/sh", "sh", "-c", cmdstring, (char *)0);  
        _exit(127); // exec执行失败返回127，注意exec只在失败时才返回现在的进程，成功的话现在的进程就不存在啦~~  
    }  
    else //父进程  
    {  
        while(waitpid(pid, &status, 0) < 0)  
        {  
            if(errno != EINTR)  
            {  
                status = -1; //如果waitpid被信号中断，则返回-1  
                break;  
            }  
        }  
    }  
    return status; //如果waitpid成功，则返回子进程的返回状态  
}  


typedef void (*sighandler_t)(int);
int safe_system(const char *cmdstring)
{
    int status = 0;
    sighandler_t old_handler;

    if(NULL == cmdstring)
    {
       return -1;
    }

    old_handler = signal(SIGCHLD, SIG_DFL);
    status = _system(cmdstring);
    signal(SIGCHLD, old_handler);

    if(status < 0)
    {
        printf_err("cmd: %s\t error: %s", cmdstring, strerror(errno)); 
        return -1;
    }

    if(WIFEXITED(status))
    {
        /* 正常执行， 取得cmdstring执行结果 */
        printf_debug("normal termination, exit status = %d\n", WEXITSTATUS(status)); 
    }
    else if(WIFSIGNALED(status))
    {
        /* 如果cmdstring被信号中断，取得信号值 */
        printf_info("abnormal termination,signal number =%d\n", WTERMSIG(status)); 
    }
    else if(WIFSTOPPED(status))
    {
        /* 如果cmdstring被信号暂停执行，取得信号值 */
        printf_info("process stopped, signal number =%d\n", WSTOPSIG(status)); 
    }

    return status;
}

