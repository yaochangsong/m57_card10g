#ifndef _UTILS_H_H_
#define _UTILS_H_H_


/*向上取整*/
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d)) 
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))

#define TIME_ELAPSED(codeToTime) do{      \
        struct timeval beginTime, endTime; \
        gettimeofday(&beginTime, NULL); \
        {codeToTime;}                   \
        gettimeofday(&endTime, NULL);   \
        long secTime  = endTime.tv_sec - beginTime.tv_sec; \
        long usecTime = endTime.tv_usec - beginTime.tv_usec; \
        printf("[%s(%d)]Elapsed Time: SecTime = %lds, UsecTime = %ldus!\n", __FUNCTION__, __LINE__, secTime, usecTime); \
}while(0)



extern char *safe_strdup(const char *s);
extern uint16_t crc16_caculate(uint8_t *pchMsg, uint16_t wDataLen);
extern int get_mac(char * mac, int len_limit);
extern int write_file_in_int16(void *pdata, unsigned int data_len, char *filename);

#endif

