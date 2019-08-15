#ifndef _UTILS_H_H_
#define _UTILS_H_H_


/*向上取整*/
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d)) 
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
#define EVEN(x) (((x)%2== 0) ? 1 : 0)


#define TIME_ELAPSED(codeToTime) do{      \
        struct timeval beginTime, endTime; \
        gettimeofday(&beginTime, NULL); \
        {codeToTime;}                   \
        gettimeofday(&endTime, NULL);   \
        long secTime  = endTime.tv_sec - beginTime.tv_sec; \
        long usecTime = endTime.tv_usec - beginTime.tv_usec; \
        printf_debug("Elapsed Time: SecTime = %lds, UsecTime = %ldus!\n", secTime, usecTime); \
}while(0)


#define FLOAT_TO_SHORT(f, s, n)  do{                            \
       float *_tf = f; signed short *_ts = s; int32_t _n = n;       \ 
                         while(_n>0){                           \
                     *_ts++ = (signed short)*_tf++; _n--;       \
    }}while(0)

extern char *safe_strdup(const char *s);
extern uint16_t crc16_caculate(uint8_t *pchMsg, uint16_t wDataLen);
extern int get_mac(char * mac, int len_limit);
extern int write_file_in_int16(void *pdata, unsigned int data_len, char *filename);
extern int32_t inline diff_time(void);
#endif

