#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <dirent.h>
#include <libgen.h>
#include "bottom.h"
#include "../../../log/log.h"
#include "../../../platform.h"



struct freq_index_t{
    int index;
    long long mfreq;
};

static inline bool str_to_llong(char *str, long long  *ivalue, bool(*_check)(int))
{
    char *end;
    long long value;
    
    if(str == NULL || ivalue == NULL)
        return false;
    
    value = (long long) strtoll(str, &end, 10);
    if (str == end){
        return false;
    }
    *ivalue = value;
    if(*_check == NULL){
         //printf("null func\n");
         return true;
    }
       
    return ((*_check)(value));
}

static ssize_t read_file_whole(void *pdata, char *filename)
{
        
    FILE *file;
    size_t fsize;
   // unsigned int *pdata_offset;

    if(pdata == NULL){
        return -1;
    }

    file = fopen(filename, "r");
    if(!file){
        printf("Open file error!\n");
        return -1;
    }
    fseek(file, 0, SEEK_END);
    fsize = ftell(file);
    rewind(file);
    fread(pdata,1, fsize, file);
    fclose(file);

    return fsize;
}

static void _bubble_llong_sort(struct freq_index_t *ptr,int n)
{
    int i,j;
    long long temp;

    for(i = 0;i < n - 1;i++)
        for(j = 0;j<n - 1- i;j++)
        {
            if(ptr[j].mfreq < ptr[j+1].mfreq){
                temp = ptr[j].mfreq;
                ptr[j].mfreq = ptr[j+1].mfreq;
                ptr[j + 1].mfreq = temp;
            }
        }
}


/*
1st dir /etc
2nd   3rd
bd->1000000->256
           ->512 
  ->2000000->256

*/


#define MAX_CHANNEL_LEVEL 2
#define MAX_FFT_LEVEL 8
#define MAX_FREQ_LEVEL 48
#define MAX_BW_LEVEL 10

#define BOTTOW_FILE_DIR "/etc/bd"

void *fft_filter_ptr[MAX_CHANNEL_LEVEL][MAX_FFT_LEVEL][MAX_FREQ_LEVEL][MAX_BW_LEVEL] = {NULL};

struct freq_index_t freq_table[48];

static struct points_i{
    int index;
    size_t  fft_size;
}fft_points[] = {
    {0, 256},
    {1, 512},
    {2, 1024},
    {3, 2048},
    {4, 4096},
    {5, 8192},
    {6, 16384},
};

static struct bw_points_i{
    int index;
    size_t  bw_mhz;
}bw_points[] = {
    {0, 1},
    {1, 2},
    {2, 5},
    {3, 10},
    {4, 20},
    {5, 40},
    {6, 80},
    {7, 160},
    {8, 175},
};


static size_t bottom_get_fftsize_by_index(size_t   index)
{
    if(index >= ARRAY_SIZE(fft_points))
        return -1;
    
    return fft_points[index].fft_size;
}

static int bottom_get_index_by_fftsize(int fftsize)
{
    for(int i = 0; i < ARRAY_SIZE(fft_points); i++){
        if(fftsize == fft_points[i].fft_size){
            return fft_points[i].index;
        }
    }
    return -1;
}


static int bottom_get_index_by_freq(long long freq)
{
    for(int i = 0; i < ARRAY_SIZE(freq_table); i++){
        if(freq == freq_table[i].mfreq){
            return freq_table[i].index;
        }
    }
    
    for(int i = 0; i < ARRAY_SIZE(freq_table) -1; i++){
        if(freq < freq_table[i].mfreq && freq > freq_table[i + 1].mfreq){
            return freq_table[i].index;
        }
    }

    return 0;
}

static int bottom_get_index_by_bw(uint32_t bw_hz)
{
    for(int i = 0; i < ARRAY_SIZE(bw_points); i++){
        if(bw_hz == MHZ(bw_points[i].bw_mhz)){
            return bw_points[i].index;
        }
    }
    
    for(int i = 0; i < ARRAY_SIZE(bw_points) -1; i++){
        if(bw_hz > bw_points[i].bw_mhz && bw_hz < bw_points[i + 1].bw_mhz){
            return bw_points[i + 1].index;
        }
    }

    return -1;
}


static uint32_t bottom_get_bw_by_index(int index)
{
    for(int i = 0; i < ARRAY_SIZE(bw_points); i++){
        if(index == bw_points[i].index){
            return MHZ(bw_points[i].bw_mhz);
        }
    }
    return 0;
}


static long long bottom_get_freq_by_index(int index)
{
    for(int i = 0; i < ARRAY_SIZE(freq_table); i++){
        if(index == freq_table[i].index){
            return freq_table[i].mfreq;
        }
    }
    return 0;
}


static char  *_bottom_noise_data_filename(char *buffer, int len, int fftisze, long long freq, uint32_t bw, int ch)
{
    if(buffer && len > 0)
        snprintf(buffer, len -1, "%s/%lld/fft_%d_%d_%u",BOTTOW_FILE_DIR, freq, ch, fftisze, bw);
        
    return buffer;
}


static int bottom_load_buffer(int ch, int fft_order_len)
{
    char filename[256];
    ssize_t ret = 0;
    struct stat fstat;
    int fftsize = 0, result = 0;;

    for(int j = 0; j < ARRAY_SIZE(freq_table); j++){
        if(freq_table[j].mfreq == 0)
            continue;
        for(int k = 0; k < ARRAY_SIZE(bw_points); k++){
            for(int i = 0; i < MAX_FFT_LEVEL; i++){
                if((fftsize = bottom_get_fftsize_by_index(i)) > 0){
                    memset(filename, 0, sizeof(filename));
                    _bottom_noise_data_filename(filename, sizeof(filename), fftsize, freq_table[j].mfreq, MHZ(bw_points[k].bw_mhz), ch);
                    printf_debug("filename: %s\n", filename);
                    if (access(filename, F_OK)) {
                        continue;
                    }
                    result = stat(filename, &fstat);
                    if(result != 0){
                        printfw("filename: %s;", filename);
                        perror("read ");
                        continue;
                    }
                    printf_debug("filename:%s, size = %ld\n", filename, fstat.st_size);
                    if(fft_order_len > 0 && fstat.st_size != fft_order_len * sizeof(fft_t)){
                        printf_debug("file size[%ld] is error, fftlen:%d,  sizelen%ld\n", fstat.st_size,  fft_order_len, fft_order_len * sizeof(fft_t));
                        continue;
                    }
                    if(fft_filter_ptr[ch][i][j][k]){
                        free(fft_filter_ptr[ch][i][j][k]);
                        fft_filter_ptr[ch][i][j][k] = NULL;
                    }
                    fft_filter_ptr[ch][i][j][k] = calloc(1, fstat.st_size);
                    ret = read_file_whole(fft_filter_ptr[ch][i][j][k], filename);
                    if(ret != fstat.st_size){
                        printf_warn("%s [%ld]read err!\n", filename, ret);
                    }
                    printf_note("ch:%d, i=%d, j=%d, k=%d, %p\n", ch, i, j, k, fft_filter_ptr[ch][i][j][k]);
                }
            }
        }
    }
    /* load data */
    
    return 0;
}


static int load_freq_table(void)
{

    DIR *pdir = NULL;
    struct dirent *pent = NULL;
    size_t count = 0;
    struct freq_index_t *ptable= NULL;
    long long freq;
    
    pdir = opendir(BOTTOW_FILE_DIR);
    if(pdir == NULL){
        perror("opendir");
        return -1;
    }
    printf_debug("read file: \n");
    while(1){
        pent = readdir(pdir);
        if(pent == NULL){
            break;
        }
        if(pent->d_type == DT_DIR){
            if(str_to_llong(pent->d_name, &freq, NULL) == false)
                continue;
            printf_debug("%s[%llu]\n", pent->d_name, freq);
            freq_table[count].index = count;
            freq_table[count].mfreq = freq;
            count++;
            if(count >= ARRAY_SIZE(freq_table)){
                printf_warn("file count[%ld] is bigger than or equal to table: %ld\n", count, ARRAY_SIZE(freq_table));
                break;
            }
        }
    }
    _bubble_llong_sort(freq_table, ARRAY_SIZE(freq_table));
    return 0;
}

static inline int _create_path_mkdir(char *path)
{
    char *path_dup = strdup(path);
    char *dir;
    //printf("[%d], %s\n", __LINE__, path_dup);
    dir = dirname(path_dup);
    if(dir == NULL)
        goto exit;
    //printf("[%d], dir:%s\n", __LINE__, dir);
    if(dir && access(dir, F_OK)){
        printf_debug("mkdir %s\n", dir);
        mkdir(dir, 0755);
    }
exit:
    if(path_dup)
        free(path_dup);
    
    return 0;
}

static inline int _create_mkdir(char *dir)
{
    printf_debug("[%d], dir:%s\n", __LINE__, dir);
    if(dir && access(dir, F_OK)){
        printf_debug("mkdir %s\n", dir);
        mkdir(dir, 0755);
    }

    return 0;
}

static void _bottom_create_avg_data(fft_t * data, size_t data_len, fft_t *avg_data)
{
    int64_t total_bn = 0;
    int32_t _bottom_noise =0,  _delta = 0;
    fft_t *ptr = data;
    fft_t *ptr_avg = avg_data;
    
    for(int i = 0; i < data_len; i++){
        total_bn += *ptr++;
    }
    _bottom_noise = total_bn/(int64_t)data_len;
    printf_note("_bottom_noise = %d, total_bn=%ld, %ld, %ld\n", _bottom_noise, total_bn, data_len, total_bn/(int64_t)data_len);
    ptr = data;
    
    for(int i = 0; i < data_len; i++){
        _delta = *ptr - _bottom_noise;
        *ptr_avg++ = _delta;
        if(i < 10){
            printfn("%d ", _delta);
        }
        ptr ++;
    }
}

static int bottom_create_file_data(int ch, void *data, int fft_order_size, int fft_size, long long freq, uint32_t bw)
{
    char filepath[256];
    ssize_t ret = 0;
    FILE *file;
    fft_t buffer[fft_order_size];
    
    if(ch >= MAX_CHANNEL_LEVEL)
        return -1;
    
    printf_debug("create file: fft_size:%d, freq:%lld\n", fft_size, freq);
    memset(filepath, 0, sizeof(filepath));
    _bottom_noise_data_filename(filepath, sizeof(filepath), fft_size, freq, bw, ch);
    _create_path_mkdir(filepath);
    _bottom_create_avg_data(data, fft_order_size, buffer);
    file = fopen(filepath, "w+");
    if(!file){
        printf("Open file[%s] error!\n", filepath);
        return -1;
    }
    ret = fwrite((void *)buffer, sizeof(fft_t), fft_order_size, file);
    if(ret != fft_order_size){
        printf_err("write err, ret:%ld, %d\n", ret, fft_order_size);
    }
    fclose(file);
    load_freq_table();
    bottom_load_buffer(ch, -1);
    
    return 0;
}


void bottom_deal(int ch, fft_t *data, size_t data_len, size_t fft_size, long long mfreq, uint32_t bw)
{
    fft_t *ptr = data;
    fft_t *filter_ptr;
    static int i_dup[MAX_CHANNEL_LEVEL] = {-1, -1}, j_dup[MAX_CHANNEL_LEVEL] = {-1, -1}, k_dup[MAX_CHANNEL_LEVEL] = {-1, -1};
    
    if(ch >= MAX_CHANNEL_LEVEL)
        return;
    
    int i = bottom_get_index_by_fftsize(fft_size);
    int j = bottom_get_index_by_freq(mfreq);
    int k = bottom_get_index_by_bw(bw);

    filter_ptr = fft_filter_ptr[ch][i][j][k];
    if(filter_ptr == NULL){
        printf_info("ch:%d, i:%d, j:%d, k:%d, freq:%lld, bw:%u, fft_size:%ld, %p\n",ch,  i, j, k, mfreq, bw, fft_size, fft_filter_ptr[ch][i][j][k]);
        printf_info("filter data is null\n");
        return;
    }
    if(i != i_dup[ch] || j != j_dup[ch] || k != k_dup[ch]){
        printf_note("====>Freq: %lld, fft_size: %lu, bw=%u\n", bottom_get_freq_by_index(j), fft_size, bw);
        i_dup[ch] = i;
        j_dup[ch] = j;
        k_dup[ch] = k;
    }

    ptr = data;
    for(int i = 0; i < data_len; i++){
        *ptr = *ptr - *filter_ptr;
        ptr ++;
        filter_ptr++;
    }
}

int bottom_calibration(int ch, void *data, size_t data_len, int fftsize, long long mfreq, uint32_t bw)
{
    static size_t s_fftsize[MAX_CHANNEL_LEVEL] = {0,0};
    static  long long s_freq[MAX_CHANNEL_LEVEL] = {0, 0};
    static  long long s_bw[MAX_CHANNEL_LEVEL] = {0, 0};
    
    if(fftsize == s_fftsize[ch] && mfreq == s_freq[ch] && bw == s_bw[ch])
        return -1;
    
    s_fftsize[ch] = fftsize;
    s_freq[ch] = mfreq;
    s_bw[ch] = bw;
    return bottom_create_file_data(ch, data, data_len, fftsize, mfreq, bw);
}

static void bottom_freq_dump(void)
{
    for(int i = 0; i < ARRAY_SIZE(freq_table); i++){
        printf("index:%d. freq=%llu\n", freq_table[i].index, freq_table[i].mfreq);
    }
}

void bottom_init(void)
{
    printf_note("Bottom Init...\n");
    _create_mkdir(BOTTOW_FILE_DIR);
    load_freq_table();
    for(int ch = 0; ch < MAX_CHANNEL_LEVEL; ch++)
        bottom_load_buffer(ch, -1);
    //bottom_freq_dump();
}

void main_test()
{
    char buffer[65536];
    long long freq = 1000000;
    bottom_init();
    #if 0
    for(int i = 0; i < ARRAY_SIZE(fft_points); i++){
        sprintf(buffer, "%ld ", fft_points[i].fft_size);
        for(int j = 1; j < 8; j ++)
            for(int ch = 0; ch < MAX_CHANNEL_LEVEL; ch++)
            bottom_create_file_data(ch, buffer, fft_points[i].fft_size/1.28, fft_points[i].fft_size, freq * j);
    }
    #endif
    bottom_freq_dump();
   // bottom_deal(0, (fft_t *)buffer, 1024/1.28, 1024, 5100000);
   // bottom_deal(1, (fft_t *)buffer, 2048/1.28, 2048, 19000000);
}

