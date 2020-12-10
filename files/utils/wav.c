#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include "wav.h"


#define WAV_FORMAT_PCM  (0x0001)
#define AUDIO_CH_NUM    (2)
#define SAMPLE_RATE     (32000)
#define SAMPLE_BITS     (16)

#define BLOCK_SIZE      (512)



/*---------------------------------------------
    参数：
        formatType：音频格式（PCM 0x0001）
        channelCount: 声道数（2）
        sampleRate：采样率（32k）
        bitsPerSample：样点位数（16）
        datalen：写入的原始数据长度（字节）
-----------------------------------------------*/

void wav_format_header(Wav *header, uint16_t formatType, uint16_t channelCount, 
                        uint32_t sampleRate, uint16_t bitsPerSample, uint64_t datalen)
{
    header->riff.ChunkID[0] = 'R';
    header->riff.ChunkID[1] = 'I';
    header->riff.ChunkID[2] = 'F';
    header->riff.ChunkID[3] = 'F';
    header->riff.ChunkSize = 0xffffffff;
    header->riff.Format[0] = 'W';
    header->riff.Format[1] = 'A';
    header->riff.Format[2] = 'V';
    header->riff.Format[3] = 'E';

    header->fmt.Subchunk1ID[0] = 'f';
    header->fmt.Subchunk1ID[1] = 'm';
    header->fmt.Subchunk1ID[2] = 't';
    header->fmt.Subchunk1ID[3] = ' ';
    header->fmt.Subchunk1Size = 16;
     header->fmt.AudioFormat = formatType;
    header->fmt.NumChannels = channelCount;
    header->fmt.SampleRate = sampleRate;
    header->fmt.ByteRate = sampleRate * channelCount * (bitsPerSample / 8);
    header->fmt.BlockAlign = channelCount * (bitsPerSample / 8);
    header->fmt.BitsPerSample = bitsPerSample;

    header->data.Subchunk3ID[0] = 'd';
    header->data.Subchunk3ID[1] = 'a';
    header->data.Subchunk3ID[2] = 't';
    header->data.Subchunk3ID[3] = 'a';
    header->data.Subchunk3Size = 0xffffffff;

    uint64_t data_max_len = header->riff.ChunkSize - sizeof(Wav) + 8;
    if (datalen <= data_max_len) {
        header->data.Subchunk3Size = datalen;
        header->riff.ChunkSize = header->data.Subchunk3Size + sizeof(Wav) - 8;
    } else {
        header->data.Subchunk3Size = data_max_len;
    }
}


void wav_write_header_before(int fd)
{
    lseek(fd, 512, SEEK_SET);
}
int wav_write_header(int fd, uint64_t datalen)
{
    int write_len = -1;
    int header_len = sizeof(Wav);
    Wav header;
    int pagesize = 0;
    void *user_mem = NULL;

    if (fd < 0){
        return -1;
    }
    pagesize = getpagesize();
    posix_memalign((void **)&user_mem, pagesize, BLOCK_SIZE + pagesize);
    if (!user_mem) {
        fprintf(stderr, "memalign failed\n");
        return -1;
    }
    lseek(fd, 0, SEEK_SET);
    memset(user_mem, 0, BLOCK_SIZE); 
    wav_format_header(&header, WAV_FORMAT_PCM, AUDIO_CH_NUM, SAMPLE_RATE, SAMPLE_BITS, datalen+BLOCK_SIZE-header_len); //为保持对齐，除去头外，填充部分数据0
    memcpy(user_mem, &header, header_len);
    write_len = write(fd, user_mem, BLOCK_SIZE);
    if (user_mem)
        free(user_mem);
    if (write_len != BLOCK_SIZE) {
        perror("wav write header");
        return -1;
    }
    return 0;
}


