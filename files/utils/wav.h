#ifndef __WAVE_H_
#define __WAVE_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/*
    标准WAV头
*/

typedef struct WAV_RIFF {
    char ChunkID[4];                /* "RIFF" */
    uint32_t ChunkSize;             /* 36 + Subchunk3Size */
    char Format[4];                 /* "WAVE" */
}__attribute__ ((packed)) RIFF_t; 

typedef struct WAV_FMT {
    char Subchunk1ID[4];            /* "fmt " */
    uint32_t Subchunk1Size;         /* 16 for PCM */
    uint16_t AudioFormat;           /* PCM = 1*/
    uint16_t NumChannels;           /* Mono = 1, Stereo = 2, etc. */
    uint32_t SampleRate;            /* 8000, 44100, etc. */
    uint32_t ByteRate;              /* = SampleRate * NumChannels * BitsPerSample/8 */
    uint16_t BlockAlign;            /* = NumChannels * BitsPerSample/8 */
    uint16_t BitsPerSample;         /* 8bits, 16bits, etc. */
}__attribute__ ((packed)) FMT_t;  

//附加头
typedef struct WAV_FACT {
    char Subchunk2ID[4];            /* "fact" */
    uint32_t Subchunk2Size;         /* data size */
    uint32_t SampleTotal;           /* The total number of sampling */
}__attribute__ ((packed)) FACT_t;  


typedef struct WAV_DATA {

    char Subchunk3ID[4];            /* "data" */
    uint32_t Subchunk3Size;         /* data size */
    //data
}__attribute__ ((packed)) Data_t;


typedef struct WAV_FORMAT {
   RIFF_t riff;        //12 bytes
   FMT_t fmt;          //24 bytes
   //FACT_t fact;      //12 bytes
   Data_t data;        //8 bytes
}__attribute__ ((packed)) Wav;

extern void wav_write_header_before(int fd);
extern int wav_write_header(int fd, uint64_t datalen);
#endif


