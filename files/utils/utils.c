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
#define _GNU_SOURCE
#include <sched.h>
#include "config.h"
#include <sys/ioctl.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <net/if.h>  
#include <error.h>  
#include <net/route.h>  


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

int get_ipaddress(struct in_addr *addr)
{
    int sock;
    struct sockaddr_in sin;
    struct ifreq ifr;
	char *temp_ip = NULL;
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
    {
        perror("socket");
        return -1;                
    }
    strncpy(ifr.ifr_name, NETWORK_EHTHERNET_POINT, IFNAMSIZ-1);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;
    
    if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)
    {
        perror("ioctl");
        close(sock);
        return -1;
    }
    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    temp_ip = inet_ntoa(sin.sin_addr);
    *addr = sin.sin_addr;
    //strcpy(ip,temp_ip);
    //fprintf(stdout, "eth0: ip %s\n", temp_ip);
    close(sock);
    return 0;
}

int get_netmask(struct in_addr *netmask)
{
    int sock;
    struct sockaddr_in sin;
    struct ifreq ifr;
    char *temp_netmask = NULL;
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
    {
        perror("socket");
        return -1;                
    }
    strncpy(ifr.ifr_name, NETWORK_EHTHERNET_POINT, IFNAMSIZ-1);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;
    
    if (ioctl(sock, SIOCGIFNETMASK, &ifr) < 0)
    {
        perror("ioctl");
        close(sock);
        return -1;
    }
    memcpy(&sin, &ifr.ifr_netmask, sizeof(sin));
    temp_netmask = inet_ntoa(sin.sin_addr);
    *netmask = sin.sin_addr;
    //strcpy(ip,temp_ip);
   // fprintf(stdout, "netmask: %s\n", temp_netmask);
    close(sock);
    return 0;
}

int get_gateway(struct in_addr * gw)
{
    long destination, gateway;
    char iface[IF_NAMESIZE];
    char buf[256];
    FILE * file;
    struct sockaddr_in sin;
    char *temp_gw = NULL;

    memset(iface, 0, sizeof(iface));
    memset(buf, 0, sizeof(buf));

    file = fopen("/proc/net/route", "r");
    if (!file)
        return -1;

    while (fgets(buf, sizeof(buf), file)) {
        if (sscanf(buf, "%s %lx %lx", iface, &destination, &gateway) == 3) {
            if (destination == 0) { /* default */
                if(!strcmp(NETWORK_EHTHERNET_POINT, iface)){
                    sin.sin_addr.s_addr = gateway;
                    temp_gw = inet_ntoa(sin.sin_addr);
                    *gw = sin.sin_addr;
                    //fprintf(stdout, "gateway: %s\n", temp_gw);
                }
                fclose(file);
                return 0;
            }
        }
    }

    /* default route not found */
    if (file)
        fclose(file);
    return -1;
}


int set_ipaddress(char *ipaddr, char *mask,char *gateway)  
{  
    int fd;  
    int rc;  
    struct ifreq ifr;   
    struct sockaddr_in *sin;  
    struct rtentry  rt;  
  
    fd = socket(AF_INET, SOCK_DGRAM, 0);  
    if(fd < 0)  
    {  
            perror("socket   error");       
            return -1;       
    }  
    memset(&ifr,0,sizeof(ifr));   
    strcpy(ifr.ifr_name,NETWORK_EHTHERNET_POINT);   
    sin = (struct sockaddr_in*)&ifr.ifr_addr;       
    sin->sin_family = AF_INET;       
    //IP鍦板潃  
    if(inet_aton(ipaddr,&(sin->sin_addr)) < 0)     
    {       
        perror("inet_aton   error");       
        return -2;       
    }      
  
    if(ioctl(fd,SIOCSIFADDR,&ifr) < 0)     
    {       
        perror("ioctl   SIOCSIFADDR   error");       
        return -3;       
    }  
    //瀛愮綉鎺╃爜  
    if(inet_aton(mask,&(sin->sin_addr)) < 0)     
    {       
        perror("inet_pton   error");       
        return -4;       
    }      
    if(ioctl(fd, SIOCSIFNETMASK, &ifr) < 0)  
    {  
        perror("ioctl");  
        return -5;  
    }  
    //缃戝叧  
    memset(&rt, 0, sizeof(struct rtentry));  
    memset(sin, 0, sizeof(struct sockaddr_in));  
    sin->sin_family = AF_INET;  
    sin->sin_port = 0;  
    if(inet_aton(gateway, &sin->sin_addr)<0)  
    {  
       printf_err( "inet_aton error\n" );  
    }  
    memcpy ( &rt.rt_gateway, sin, sizeof(struct sockaddr_in));  
    ((struct sockaddr_in *)&rt.rt_dst)->sin_family=AF_INET;  
    ((struct sockaddr_in *)&rt.rt_genmask)->sin_family=AF_INET;  
    rt.rt_flags = RTF_GATEWAY;  
    if (ioctl(fd, SIOCADDRT, &rt)<0)  
    {  
        printf_err( "ioctl(SIOCADDRT) error in set_default_route\n");  
        close(fd);  
        return -1;  
    }  
    close(fd);  
    return rc;  
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

    fread(pdata,data_len,1,file);
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

int32_t  diff_time(void)
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


char *get_build_time(void)
{   /*__DATE__   Nov 29 2019  ==>  2019-11-29*/
    static char data[32];
    #define YEAR ((((__DATE__ [7] - '0') * 10 + (__DATE__ [8] - '0')) * 10 \
                + (__DATE__ [9] - '0')) * 10 + (__DATE__ [10] - '0'))
    #define MONTH (__DATE__ [2] == 'n' ? (__DATE__ [1] == 'a' ? 0 : 5)  \
    : __DATE__ [2] == 'b' ? 1 \
    : __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 2 : 3) \
    : __DATE__ [2] == 'y' ? 4 \
    : __DATE__ [2] == 'l' ? 6 \
    : __DATE__ [2] == 'g' ? 7 \
    : __DATE__ [2] == 'p' ? 8 \
    : __DATE__ [2] == 't' ? 9 \
    : __DATE__ [2] == 'v' ? 10 : 11)

    #define DAY ((__DATE__ [4] == ' ' ? 0 : __DATE__ [4] - '0') * 10 \
            + (__DATE__ [5] - '0'))
    #define DATE_AS_INT (((YEAR - 2000) * 12 + MONTH) * 31 + DAY)
    
    sprintf(data, "%d-%02d-%02d", YEAR, MONTH + 1, DAY);
    return data;
}

char *get_jenkins_version(void)
{
#ifdef SUPPORT_PLATFORM_ARCH_ARM
    #define J_FILE_NAME "/etc/JENKINS_VERSION"
#else
    #define J_FILE_NAME "conf/JENKINS_VERSION"
#endif
    static char version[16] = {"0"};
    
    read_file(version, sizeof(version), J_FILE_NAME);
    version[sizeof(version)-1] = 0;
    return version;
}

char *get_version_string(void)
{
   static char version[64]={0};
   sprintf(version, "%s(%s-%s)", PLATFORM_VERSION,get_build_time(), __TIME__);
  // sprintf(version, "%s_%s(%s-%s)", PLATFORM_VERSION,get_jenkins_version(), get_build_time(), __TIME__);
   printf_debug("%s\n", version);
   return version;
}



/* 将线程绑定到某个CPU上 */
int thread_bind_cpu(int cpuindex)
{
    cpu_set_t mask;
    cpu_set_t get;
    int j;
    /* 获取系统CPU的个数 */
    int cpunum = sysconf(_SC_NPROCESSORS_CONF);
    printf_note("system has %d processor(s)\n", cpunum);
    if(cpuindex > cpunum){
        printf_err("system processor(s) number %d is less than bind index[%d]\n", cpunum, cpuindex);
        return -1;
    }
    CPU_ZERO(&mask);
    CPU_SET(cpuindex, &mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) {
            printf_err("set thread affinity failed\n");
            return -1;
    }
    CPU_ZERO(&get);
    if (pthread_getaffinity_np(pthread_self(), sizeof(get), &get) < 0) {
        printf_err("get thread affinity failed\n");
        return -1;
    }
    for (j = 0; j < cpunum; j++) {
        if (CPU_ISSET(j, &get)) {
            printf_note("thread %u is running in processor %d\n", pthread_self(), j);
        }
    }
    return 0;
}


