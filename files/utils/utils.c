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
#include <sys/sysinfo.h>


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

int get_ipaddress(char *ifname, struct in_addr *addr)
{
    int sock;
    struct sockaddr_in sin;
    struct ifreq ifr;
	//char *temp_ip = NULL;
    if(ifname == NULL)
        return -1;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
    {
        perror("socket");
        return -1;                
    }
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ-1);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;
    
    if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)
    {
        perror("ioctl");
        close(sock);
        return -1;
    }
    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    //temp_ip = inet_ntoa(sin.sin_addr);
    //*addr = sin.sin_addr;
    memcpy(addr, &sin.sin_addr, sizeof(struct sockaddr_in));
    //strcpy(ip,temp_ip);
    //fprintf(stdout, "eth0: ip %s\n", temp_ip);
    close(sock);
    return 0;
}

int get_netmask(char *ifname, struct in_addr *netmask)
{
    int sock;
    struct sockaddr_in sin;
    struct ifreq ifr;
    //char *temp_netmask = NULL;
    if(ifname == NULL)
        return -1;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
    {
        perror("socket");
        return -1;                
    }
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ-1);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;
    
    if (ioctl(sock, SIOCGIFNETMASK, &ifr) < 0)
    {
        perror("ioctl");
        close(sock);
        return -1;
    }
    memcpy(&sin, &ifr.ifr_netmask, sizeof(sin));
    //temp_netmask = inet_ntoa(sin.sin_addr);
   // *netmask = sin.sin_addr;
    memcpy(netmask, &sin.sin_addr, sizeof(struct sockaddr_in));
    //strcpy(ip,temp_ip);
   // fprintf(stdout, "netmask: %s\n", temp_netmask);
    close(sock);
    return 0;
}

int get_gateway(char *ifname, struct in_addr * gw)
{
    long destination, gateway;
    char iface[IF_NAMESIZE];
    char buf[256];
    FILE * file;
    struct sockaddr_in sin;
   // char *temp_gw = NULL;
    
    if(ifname == NULL || gw == NULL)
        return -1;
    memset(iface, 0, sizeof(iface));
    memset(buf, 0, sizeof(buf));

    file = fopen("/proc/net/route", "r");
    if (!file)
        return -1;

    while (fgets(buf, sizeof(buf), file)) {
        if (sscanf(buf, "%s %lx %lx", iface, &destination, &gateway) == 3) {
            if (destination == 0) { /* default */
                if(!strcmp(ifname, iface)){
                    sin.sin_addr.s_addr = gateway;
                    //temp_gw = inet_ntoa(sin.sin_addr);
                    //*gw = sin.sin_addr;
                    memcpy(gw, &sin.sin_addr, sizeof(struct sockaddr_in));
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

#include  <linux/ethtool.h>
int32_t get_netlink_status(const char *if_name)
{
#ifndef SIOCETHTOOL
#define SIOCETHTOOL     0x8946
#endif
    int skfd;
    struct ifreq ifr;
    struct ethtool_value edata;
    edata.cmd = ETHTOOL_GLINK;
    edata.data = 0;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_data = (char *) &edata;
    if (( skfd = socket( AF_INET, SOCK_DGRAM, 0 )) == 0)
        return -1;
    if(ioctl( skfd, SIOCETHTOOL, &ifr ) == -1)
    {
        close(skfd);
        return -1;
    }
    close(skfd);
    return edata.data;
}
int32_t get_ifname_speed(const char *ifname)
{
#ifndef SIOCETHTOOL
#define SIOCETHTOOL     0x8946
#endif
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif
#define ETHTOOL_GSET        0x00000001 /* Get settings. */
#define ETHTOOL_SSET        0x00000002 /* Set settings. */
typedef __uint32_t __u32;       /* ditto */
typedef __uint16_t __u16;       /* ditto */
typedef __uint8_t __u8;         /* ditto */
struct ethtool_cmd {
        __u32   cmd;
        __u32   supported;      /* Features this interface supports */
        __u32   advertising;    /* Features this interface advertises */
        __u16   speed;          /* The forced speed, 10Mb, 100Mb, gigabit */
        __u8    duplex;         /* Duplex, half or full */
        __u8    port;           /* Which connector port */
        __u8    phy_address;
        __u8    transceiver;    /* Which transceiver to use */
        __u8    autoneg;        /* Enable or disable autonegotiation */
        __u32   maxtxpkt;       /* Tx pkts before generating tx int */
        __u32   maxrxpkt;       /* Rx pkts before generating rx int */
        __u32   reserved[4];
};
    struct ifreq ifr, *ifrp;
   int fd;
   int link = -1;
#if 0
   link = get_netlink_status(ifname);
   if(link == 0){
        printf_warn("link down\n");
        return 0;
   }else if(link == -1){
        printf_warn("link status check errno\n");
        return -2;
   }
   printf_warn("link up\n");
#endif
   memset(&ifr, 0, sizeof(ifr));
   strcpy(ifr.ifr_name, ifname);
   fd = socket(AF_INET, SOCK_DGRAM, 0);
   if (fd < 0) {
       perror("Cannot get control socket");
       return -2;
   }
   int err;
   struct ethtool_cmd ep;
   ep.cmd = ETHTOOL_GSET;
   ifr.ifr_data = (caddr_t)&ep;
   err = ioctl(fd, SIOCETHTOOL, &ifr);
   if (err != 0) {
       close(fd);
       //printf(" ioctl is erro .\n");
       return -2;
   }
   close(fd);
   printf_note("%s Speed: %dMb\n", ifname , ep.speed);
    return ep.speed;
}

int get_ifname_number(void)
{
    int num = 0;
    struct if_nameindex *head, *ifni;
    ifni = if_nameindex();
    head = ifni;

    if(head == NULL){
        perror("if_nameindex()");
        return num;
    }

    while(ifni->if_index != 0){
        //printf("Interface %d, %s\n", ifni->if_index, ifni->if_name);
        ifni++;
        num ++;
    }

    if_freenameindex(head);
    head = NULL;
    ifni = NULL;
    return num;
}

int set_ipaddress(char *ifname, char *ipaddr, char *mask,char *gateway)  
{  
    int fd;  
    int rc = 0;  
    struct ifreq ifr;   
    struct sockaddr_in *sin;  
    struct rtentry  rt;  

    if(ifname == NULL || ipaddr == NULL || mask== NULL || gateway == NULL){
        return -1;
    }
    fd = socket(AF_INET, SOCK_DGRAM, 0);  
    if(fd < 0)  
    {  
            perror("socket   error");       
            return -1;       
    }  
    memset(&ifr,0,sizeof(ifr));   
    strcpy(ifr.ifr_name,ifname);   
    sin = (struct sockaddr_in*)&ifr.ifr_addr;       
    sin->sin_family = AF_INET;       
    //IPåœ°å€  
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
    //å­ç½‘æŽ©ç   
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
    //ç½‘å…³  
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


int get_mac(char *ifname, uint8_t * mac, int len_limit)    
{
    struct ifreq ifreq;
    int sock;

    if(mac ==NULL)
        return -1;
    if(ifname == NULL)
        return -1;
    if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror ("socket");
        return -1;
    }
    strcpy (ifreq.ifr_name, ifname);    

    if (ioctl (sock, SIOCGIFHWADDR, &ifreq) < 0)
    {
        perror ("ioctl");
        return -1;
    }
    
    memcpy(mac, &ifreq.ifr_hwaddr.sa_data, len_limit);
    
    return 0;
}


#define CRC_16_POLYNOMIALS   0x8005
uint16_t crc16_caculate(const uint8_t *pchMsg, uint16_t wDataLen) {
    uint8_t i;
    uint8_t chChar;
    uint16_t wCRC = 0xFFFF; //g_CRC_value;
    uint8_t *ptr = pchMsg;
    while (wDataLen--) {
        chChar = *ptr++;
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
        printf_err("Open file error!\n");
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
        printf_err("Open file error!\n");
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
        printf_err("Open file error!\n");
        return -1;
    }

    //fread(pdata,data_len,1,file);
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

long get_sys_boot_time(void)
{
    long timenum;
    struct sysinfo info;
    
    if(sysinfo(&info)) {
        fprintf(stderr, "Failed to get sysinfo, errnoï¼š %u, reason:%s\n", errno, strerror(errno));
        return 0;
    }
    timenum = info.uptime;
    char run_time[128];
    int runday = timenum/86400;
    int runhour = (timenum%86400)/3600;
    int runmin = (timenum%3600)/60;
    int runsec = timenum%60;

    bzero(run_time, sizeof(run_time));
    snprintf(run_time, sizeof(run_time) - 1, "system run time[%lu]: %d DAY, %d Hour, %d Min, %d Sec.",timenum, runday, runhour, runmin, runsec);
    printf("===>%s\n", run_time);
    return timenum;
}

/*Return Format: 2020-09-24-10:08:57 */
char *get_proc_boot_time(void)
{
    struct sysinfo info;
    time_t cur_time = 0;
    time_t boot_time = 0;
    struct tm *ptm;
    static char time_buf[128] = {0};
    
    if(sysinfo(&info)) {
        fprintf(stderr, "Failed to get sysinfo, errnoï¼š %u, reason:%s\n", errno, strerror(errno));
        return NULL;
    }

    time(&cur_time);
    if(cur_time > info.uptime) {
        boot_time = cur_time - info.uptime;
    }else {
        boot_time = info.uptime - cur_time;
    }

    bzero(time_buf, sizeof(time_buf));
    ptm = gmtime(&boot_time);
    snprintf(time_buf, sizeof(time_buf) -1, "%04d-%02d-%02d-%02d:%02d:%02d", 
            ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    
    return time_buf;
}

/* system() ´æÔÚ¸¸½ø³ÌÄÚ´æ´óµÄ»°£¬´æÔÚ¿Õ¼äÉêÇëÊ§°ÜµÄÎÊÌâ£¬ÕâÀï¸ÄÐ´system() */
static int _system(const char * cmdstring)  
{  
    pid_t pid;  
    int status;  
  
    if(cmdstring == NULL)  
    {  
        return (1); //Èç¹ûcmdstringÎª¿Õ£¬·µ»Ø·ÇÁãÖµ£¬Ò»°ãÎª1  
    }  
      
    if((pid = vfork())<0)  /* system() ÕâÀï²ÉÓÃfork()µ÷ÓÃ£¬Èç¹û¸¸½ø³ÌÄÚ´æ´óµÄ»°£¬´æÔÚ¿Õ¼äÉêÇëÊ§°ÜµÄÎÊÌâ */
    {  
        status = -1; //forkÊ§°Ü£¬·µ»Ø-1  
    }  
    else if(pid == 0)  
    {  
        execl("/bin/sh", "sh", "-c", cmdstring, (char *)0);  
        _exit(127); // execÖ´ÐÐÊ§°Ü·µ»Ø127£¬×¢ÒâexecÖ»ÔÚÊ§°ÜÊ±²Å·µ»ØÏÖÔÚµÄ½ø³Ì£¬³É¹¦µÄ»°ÏÖÔÚµÄ½ø³Ì¾Í²»´æÔÚÀ²~~  
    }  
    else //¸¸½ø³Ì  
    {  
        while(waitpid(pid, &status, 0) < 0)  
        {  
            if(errno != EINTR)  
            {  
                status = -1; //Èç¹ûwaitpid±»ÐÅºÅÖÐ¶Ï£¬Ôò·µ»Ø-1  
                break;  
            }  
        }  
    }  
    return status; //Èç¹ûwaitpid³É¹¦£¬Ôò·µ»Ø×Ó½ø³ÌµÄ·µ»Ø×´Ì¬  
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
        /* Õý³£Ö´ÐÐ£¬ È¡µÃcmdstringÖ´ÐÐ½á¹û */
        printf_debug("normal termination, exit status = %d\n", WEXITSTATUS(status)); 
    }
    else if(WIFSIGNALED(status))
    {
        /* Èç¹ûcmdstring±»ÐÅºÅÖÐ¶Ï£¬È¡µÃÐÅºÅÖµ */
        printf_info("abnormal termination,signal number =%d\n", WTERMSIG(status)); 
    }
    else if(WIFSTOPPED(status))
    {
        /* Èç¹ûcmdstring±»ÐÅºÅÔÝÍ£Ö´ÐÐ£¬È¡µÃÐÅºÅÖµ */
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

char *_get_build_time(void)
{
    static char btime[256]={0};

     snprintf(btime, sizeof(btime), "%s-%s", get_build_time(), __TIME__);

     return btime;

}


char *get_kernel_version(void)
{
    FILE * fp = NULL;
    static char version[256]={0};

    if(strlen(version) == 0){
        fp = fopen ("/proc/version", "r");
        if(!fp){
            printf_err("Open file error!\n");
            goto exit;
        }
        rewind(fp);
        fgets(version, sizeof(version) - 1, fp);
        /* in case of too long */
        version[sizeof(version) - 1] = 0;
        /* remove '\n' */
        version[strlen(version) - 1] = 0;
        fclose(fp);
    }

exit:
    return version;
}

typedef enum {
    OBadOption,
    OBuildName,
    OBuildId,
    OGitUrl,
    OGitBranch,
    ORelease,
}OpCodes;

static const struct {
    const char *name;
    OpCodes opcode;
} compile_name[] = {
    {"BUILD_NAME", OBuildName},
    {"BUILD_ID", OBuildId},
    {"GIT_URL", OGitUrl},
    {"GIT_BRANCH", OGitBranch},
    {"RELEASE", ORelease},
    {NULL, OBadOption},
};

static OpCodes parse_token(const char *cp, char *filename, int linenum)
{
    int i;

    for(i = 0; compile_name[i].name; i++)
        if(strcasecmp(cp, compile_name[i].name) == 0)
            return compile_name[i].opcode;

    printf_err("%sï¼š line %d: Bad Option: %s\n", filename, linenum, cp);

    return OBadOption;
}

struct poal_compile_Info *open_compile_info(const char *filename)
{
    #define MAX_BUF 4096
    FILE * fp = NULL;
    int linenum = 0, i, opcode;
    char line[MAX_BUF] = {0}, *s, *p1, *p2;
    static struct poal_compile_Info c_info;

    fp = fopen (filename, "r");
    if(!fp){
        printf_err("Open file error!\n");
        goto next;
    }
    rewind(fp);
    while(!feof(fp) && fgets(line, MAX_BUF, fp)){
        linenum ++;
        s = line;
        if(s[strlen(s) -1] == '\n')
            s[strlen(s) -1] = '\0';

        if((p1 = strchr(s, ' '))) {
            p1[0] = '\0';
        } else if((p1 = strchr(s, '\t'))) {
            p1[0] = '\0';
        }

        if(p1){
            p1 ++;
            if((p2 = strchr(p1, ' '))){
                p2[0] = '\0';
            } else if((p2 = strstr(p1, "\r\n"))){
                p2[0] = '\0';
            } else if((p2 = strchr(p1, '\n'))){
                p2[0] = '\0';
            }
        }

        if(p1 && p1[0] != '\0'){
            if((strncmp(s, "#", 1)) != 0){
                printf_note("Parsing token: %s, value: %s\n", s, p1);
                opcode = parse_token(s, filename, linenum);

                switch(opcode){
                    case OBuildName:
                        c_info.build_name = safe_strdup(p1);
                        break;
                    case OBuildId:
                        c_info.build_jenkins_id = safe_strdup(p1);
                        break;
                    case OGitUrl:
                        c_info.code_url= safe_strdup(p1);
                        break;
                    case OGitBranch:
                        c_info.code_branch= safe_strdup(p1);
                        break;
                    case ORelease:
                        c_info.release_debug= safe_strdup(p1);
                        break;
                }
            }
        }
    }

    fclose(fp);

next:
    c_info.build_time = _get_build_time();
    c_info.build_version = PLATFORM_VERSION;
#ifdef VERSION_TAG
    c_info.code_hash = VERSION_TAG;
#endif

    return &c_info;
}


void *get_compile_info(void)
{
#ifdef SUPPORT_PLATFORM_ARCH_ARM
    #define C_FILE_NAME "/etc/compile.info"
#else
    #define C_FILE_NAME "conf/compile.info"
#endif

    static struct poal_compile_Info *pinfo = NULL;
    if(pinfo == NULL)
        pinfo  = open_compile_info(C_FILE_NAME);
    return (void *)pinfo;
}

char *get_jenkins_version(void)
{
#ifdef SUPPORT_PLATFORM_ARCH_ARM
    #define J_FILE_NAME "/etc/JENKINS_VERSION"
#else
    #define J_FILE_NAME "conf/JENKINS_VERSION"
#endif
    static char version[16] = {"0"};
    if(access(J_FILE_NAME, 0)){
        sprintf(version, "debug");
        return version;
    }
    if(read_file(version, sizeof(version), J_FILE_NAME) == 0){
        version[sizeof(version)-1] = 0;
    }else{
        sprintf(version, "debug");
    }
    
    return version;
}

char *get_version_string(void)
{
   static char version[256]={0};
   #ifdef VERSION_TAG
   snprintf(version, sizeof(version), "%s_%s(%s-%s)-%s", PLATFORM_VERSION,get_jenkins_version(), get_build_time(), __TIME__,VERSION_TAG);
   #else
   snprintf(version, sizeof(version), "%s_%s(%s-%s)", PLATFORM_VERSION,get_jenkins_version(), get_build_time(), __TIME__);
   #endif
   printf_debug("%s\n", version);
   return version;
}



/* ½«Ïß³Ì°ó¶¨µ½Ä³¸öCPUÉÏ */
int thread_bind_cpu(int cpuindex)
{
    cpu_set_t mask;
    cpu_set_t get;
    int j;
    /* »ñÈ¡ÏµÍ³CPUµÄ¸öÊý */
    long cpunum = sysconf(_SC_NPROCESSORS_CONF);
    printf_note("system has %lu processor(s)\n", cpunum);
    if(cpuindex > cpunum){
        printf_err("system processor(s) number %lu is less than bind index[%d]\n", cpunum, cpuindex);
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
            printf_note("thread %lu is running in processor %d\n", pthread_self(), j);
        }
    }
    return 0;
}


