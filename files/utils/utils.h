#ifndef _UTILS_H_H_
#define _UTILS_H_H_


/*ÏòÉÏÈ¡Õû*/
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d)) 
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
#define EVEN(x) (((x)%2== 0) ? 1 : 0)
#define alignment_down(a, size) (((uint32_t)a/(uint32_t)size)*(uint32_t)size) //((uint32_t)a & (~((uint32_t)size-1)))
#define alignment_up(a, size)   (size +((a-1)/size)*size)

/*
å•ç²¾åº¦æµ®ç‚¹æ¯”è¾ƒ
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


extern char *safe_strdup(const char *s);
extern uint16_t crc16_caculate(uint8_t *pchMsg, uint16_t wDataLen);
extern int get_mac(char * mac, int len_limit);
extern int write_file_in_int16(void *pdata, unsigned int data_len, char *filename);
extern int32_t inline diff_time(void);
#endif

