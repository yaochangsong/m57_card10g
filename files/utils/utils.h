#ifndef _UTILS_H_H_
#define _UTILS_H_H_


/*œÚ…œ»°’˚*/
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d)) 
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
#define EVEN(x) (((x)%2== 0) ? 1 : 0)
#define alignment_down(a, size) (((uint32_t)a/(uint32_t)size)*(uint32_t)size) //((uint32_t)a & (~((uint32_t)size-1)))
#define alignment_up(a, size)   (size +((a-1)/size)*size)

/*
ÂçïÁ≤æÂ∫¶ÊµÆÁÇπÊØîËæÉ
a == b                  a != b              a < b                     a > b
sgn(a - b) == 0         sgn(a - b) != 0       sgn(a - b) <  0         sgn(a - b) > 0  
*/
#define eps (1e-6)
#define f_sgn(a) (a < -eps ? -1 : a < eps ? 0 : 1)



#define TIME_ELAPSED(codeToTime) do{      \
        struct timeval beginTime, endTime; \
        gettimeofday(&beginTime, NULL); \
        {codeToTime;}                   \
        gettimeofday(&endTime, NULL);   \
        long secTime  = endTime.tv_sec - beginTime.tv_sec; \
        long usecTime = endTime.tv_usec - beginTime.tv_usec; \
        printf_note("Elapsed Time: SecTime = %lds, UsecTime = %ldus!\n", secTime, usecTime); \
}while(0)


#define FLOAT_TO_SHORT(f, s, n)  do{                            \
       float *_tf = f; signed short *_ts = s; int32_t _n = n;       \
                         while(_n>0){                           \
                     *_ts++ = (signed short)*_tf++; _n--;       \
    }}while(0)

#define SSDATA_CUT_OFFSET(data, offset, n)  do{                            \
                    signed short *_data = data; int _n = n;              \
                                      while(_n>0){                       \
                                  *_data = *_data+offset; _n--; _data++; \
                 }}while(0)

#define SSDATA_MUL_OFFSET(data, offset, n)  do{                            \
                  signed short *_data = data; int _n = n;              \
                                    while(_n>0){                       \
                                *_data = *_data*offset; _n--; _data++; \
               }}while(0)
#define FDATA_MUL_OFFSET(data, offset, n)  do{                            \
                float *_data = data; int _n = n;              \
                    while(_n>0){                       \
                *_data = *_data*offset; _n--; _data++; \
}}while(0)


#ifdef SUPPORT_PLATFORM_ARCH_ARM
#ifdef SUPPORT_NET_WZ
#define NETWORK_10G_EHTHERNET_POINT       "eth0"
#define NETWORK_EHTHERNET_POINT           "eth1"
#else
#define NETWORK_EHTHERNET_POINT       "eth0"
#endif
#else
#define NETWORK_10G_EHTHERNET_POINT   "eno2"
#define NETWORK_EHTHERNET_POINT       "eno1"
#endif

#define SPEED_10        10
#define SPEED_100       100
#define SPEED_1000      1000
#define SPEED_2500      2500
#define SPEED_10000     10000

#include <arpa/inet.h>  

extern char *safe_strdup(const char *s);
extern uint16_t crc16_caculate(const uint8_t *pchMsg, uint16_t wDataLen);
extern int get_mac(char *ifname, uint8_t * mac, int len_limit);
extern int write_file_in_int16(void *pdata, unsigned int data_len, char *filename);
extern int32_t  diff_time(void);
extern char *get_version_string(void);
extern void* safe_malloc(size_t size);
extern int safe_system(const char *cmdstring);
extern void safe_free(void *p);
extern int32_t get_ifname_speed(const char *ifname);
extern int get_ifname_number(void);
extern int32_t get_netlink_status(const char *if_name);
extern char *get_build_time(void);
extern long get_sys_boot_time(void);
extern char *get_proc_boot_time(void);
extern char *get_kernel_version(void);
extern void *get_compile_info(void);
extern int get_gateway(char *ifname, struct in_addr * gw);
extern int get_netmask(char *ifname, struct in_addr *netmask);
extern int get_ipaddress(char *ifname, struct in_addr *addr);
extern int set_ipaddress(char *ifname, char *ipaddr, char *mask,char *gateway);
extern int read_file(void *pdata, unsigned int data_len, char *filename);
extern int thread_bind_cpu(int cpuindex);
extern char *get_version(void);
extern ssize_t read_file_whole(void *pdata, char *filename);

#endif

