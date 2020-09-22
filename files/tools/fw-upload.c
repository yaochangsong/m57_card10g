#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fw-upload.h"

void file_copy_omit(char *srcname, char *dstname, int offset, int length)
{
    FILE *in, *out;
    int _len = length, i, rc;
    char buffer[64];
    if((in = fopen(srcname, "rb")) == NULL){
        exit(1);
    }
    if((out = fopen(dstname, "wb")) == NULL){
        exit(1);
    }
    rewind(in);
    fseek(in, offset, SEEK_SET);
    for(i = 0; i< length; i++){
        rc = fread(buffer, 1, 1, in);
        fwrite(buffer, 1, 1, out);
    }
    fclose(out);
    fclose(in);
}

void *memmem(const void *buf, size_t buf_len, const void *byte_line, size_t byte_line_len)
{
    unsigned char *bl = (unsigned char *)byte_line;
    unsigned char *bf = (unsigned char *)buf;
    unsigned char *p  = bf;

    while (byte_line_len <= (buf_len - (p - bf))){
        unsigned int b = *bl & 0xff;
        if ((p = (unsigned char *) memchr(p, b, buf_len - (p - bf))) != NULL){
            if ( (memcmp(p, byte_line, byte_line_len)) == 0)
                return p;
            else
                p++;
        }else{
            break;
        }
    }
    return NULL;
}


#define MEM_SIZE	1024
#define MEM_HALT	512
int findStrInFile(char *filename, int offset, const char *str, int str_len)
{
    int pos = 0, rc;
    FILE *fp;
    unsigned char mem[MEM_SIZE];

    if(str_len > MEM_HALT)
        return -1;
    if(offset <0)
        return -1;

    fp = fopen(filename, "rb");
    if(!fp)
        return -1;

    rewind(fp);
    fseek(fp, offset + pos, SEEK_SET);
    rc = fread(mem, 1, MEM_SIZE, fp);
    while(rc){
        unsigned char *mem_offset;
        mem_offset = (unsigned char*)memmem(mem, rc, str, str_len);
       // D("mem_offset=%p, rc=%d, str=%s, str_len=%d\n", mem_offset, rc, str, str_len);
        if(mem_offset){
            fclose(fp);	//found it
            return (mem_offset - mem) + pos + offset;
        }

        if(rc == MEM_SIZE){
            pos += MEM_HALT;	// 8
        }else
            break;

        rewind(fp);
        fseek(fp, offset+pos, SEEK_SET);
        rc = fread(mem, 1, MEM_SIZE, fp);
    }

    fclose(fp);
    return -1;
}

/*
 *  ps. callee must free memory...
 */
void *getMemInFile(char *filename, int offset, int len)
{
    void *result;
    FILE *fp;
    if( (fp = fopen(filename, "r")) == NULL ){
        return NULL;
    }
    fseek(fp, offset, SEEK_SET);
    
    result = malloc(sizeof(unsigned char) * len );
    if(!result)
        return NULL;
    
    if( fread(result, 1, len, fp) != len){
        free(result);
        return NULL;
    }
    return result;
}

/*
 *  taken from "mkimage -l" with few modified....
 */
int check(char *imagefile, int offset, int len, char *err_msg)
{
    struct stat sbuf;

    int  data_len;
    char *data;
    unsigned char *ptr;
    unsigned long checksum;

    image_header_t header;
    image_header_t *hdr = &header;

    int ifd;

    if ((unsigned)len < sizeof(image_header_t)) {
        sprintf (err_msg, "Bad size: \"%s\" is no valid image\n", imagefile);
        return 0;
    }

    ifd = open(imagefile, O_RDONLY);
    if(!ifd){
        sprintf (err_msg, "Can't open %s: %s\n", imagefile, strerror(errno));
        return 0;
    }

    if (fstat(ifd, &sbuf) < 0) {
        close(ifd);
        sprintf (err_msg, "Can't stat %s: %s\n", imagefile, strerror(errno));
        return 0;
    }

    ptr = (unsigned char *) mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, ifd, 0);
    if ((caddr_t)ptr == (caddr_t)-1) {
        close(ifd);
        sprintf (err_msg, "Can't mmap %s: %s\n", imagefile, strerror(errno));
        return 0;
    }
    ptr += offset;

    /*
    * compare MTD partition size and image size
    */
    if(len > getMTDPartSize("\"jffs2\"")){
        munmap(ptr, len);
        close(ifd);
        sprintf(err_msg, "*** Warning: the image file(0x%x) is bigger than Kernel_RootFS MTD partition.\n", len);
        return 0;
    }
    D("getMTDPartSize: %u\n", getMTDPartSize("\"jffs2\""));

    munmap(ptr, len);
    close(ifd);

    return 1;
}



struct image_part_type _pname[]={
    {"BOOT.BIN", "\"boot\""},
//    {"??", "bootenv"},
    {"image.ub", "\"kernel\""},
    {"rootfs.jffs2", "\"jffs2\""},
    {"system.dtb", "\"dtb\""}
};

int main (int argc, char *argv[])
{
    #define RFC_ERROR "RFC1867 error"
    #define ROOT_DIR "/run"
    int file_begin, file_end;
    int line_begin, line_end;
    char *boundary; int boundary_len;

    char buffer[512] = {0};
    int r, fd = -1,fd1 = -1;
    char *filename="upload.f.tmp";
    char *lockfile = ".fwupload.lock";

    if (chdir(ROOT_DIR))
      perror("chdir()");

    if(!access(lockfile, F_OK)){
        printf("fw upload deamon is busy\n");
        return;
    }
    
    fd1 = open(lockfile, O_CREAT, 0666);
    if(fd1 != -1)
        close(fd1);
    
    if(!access(filename, F_OK)){
        remove(filename);
    }

    if(fd == -1)
        fd = open(filename, O_CREAT | O_WRONLY, 0666);

    while((r = read(0, buffer, sizeof(buffer))) > 0){
        write(fd, buffer, r);
    } 
    if(fd != -1)
        close(fd);
    
    line_begin = 0;
    if((line_end = findStrInFile(filename, line_begin, "\r\n", 2)) == -1){
        D("%s %d\n", RFC_ERROR, 1);
        remove(lockfile);
        return -1;
    }
    D("[%d]line_end: %d\n", __LINE__, line_end);

    boundary_len = line_end - line_begin;
    boundary = getMemInFile(filename, line_begin, boundary_len);
    D("boundary=%s\n",boundary);

    char *line, *semicolon, *user_filename;
    line_begin = line_end + 2;
    if((line_end = findStrInFile(filename, line_begin, "\r\n", 2)) == -1){
        D("%s %d", RFC_ERROR, 2);
        goto err;
    }
    D("[%d]line_begin=%d, line_end: %d\n", __LINE__,line_begin, line_end);
    
    line = getMemInFile(filename, line_begin, line_end - line_begin);
    if(strncasecmp(line, "content-disposition: form-data;", strlen("content-disposition: form-data;"))){
        D("%s %d", RFC_ERROR, 3);
        goto err;
    }
     D("line=%s\n",line);

    
    semicolon = line + strlen("content-disposition: form-data;") + 1;
    D("semicolon=%s\n",semicolon);

    
    if(! (semicolon = strchr(semicolon, ';'))  ){
        printf("We dont support multi-field upload.\n");
        goto err;
    }
    user_filename = semicolon + 2;
    D("user_filename=%s\n",user_filename);
    
    if(strncasecmp(user_filename, "filename=", strlen("filename="))  ){
        printf("%s %d", RFC_ERROR, 4);
        goto err;
    }
    user_filename += strlen("filename=");
    D("user_filename=%s\n",user_filename);
    //until now we dont care about what the true filename is.
    //free(line);

    // We may check a string  "Content-Type: application/octet-stream" here,
    // but if our firmware extension name is the same with other known ones, 
    // the browser would use other content-type instead.
    // So we dont check Content-type here...
    line_begin = line_end + 2;
    if((line_end = findStrInFile(filename, line_begin, "\r\n", 2)) == -1){
        printf("%s %d", RFC_ERROR, 5);
        goto err;
    }
    
    line_begin = line_end + 2;
    if((line_end = findStrInFile(filename, line_begin, "\r\n", 2)) == -1){
        printf("%s %d", RFC_ERROR, 6);
        goto err;
    }
    
    file_begin = line_end + 2;
    
    if( (file_end = findStrInFile(filename, file_begin, boundary, boundary_len)) == -1){
        printf("%s %d", RFC_ERROR, 7);
        goto err;
    }
    file_end -= 2;        // back 2 chars.(\r\n);
#if 0
    if(!check(filename, file_begin, file_end - file_begin, err_msg) ){
        printf("Not a valid firmware. %s", err_msg);
        goto err;
    }
#endif
    user_filename = user_filename + 1;
    user_filename[strlen(user_filename) - 1] = 0;
    D("user_filename=%s\n",user_filename);

    if(!access(user_filename, F_OK)){
        remove(user_filename);
    }
    file_copy_omit(filename, user_filename, file_begin, file_end - file_begin);

    char *_mtd_name = NULL;
    for(int i = 0; i<(sizeof(_pname)/sizeof(_pname[0])); i++){
        if(!strcmp(user_filename, _pname[i].filename)){
            _mtd_name = _pname[i].mtdname;
            break;
        }
    }
    if(_mtd_name == NULL){
        printf("file name is error!");
        goto err;
    }
    D("_mtd_name=%s\n", _mtd_name);
    // flash write
    if( mtd_write_part(user_filename, _mtd_name) == -1){
        printf("mtd_write fatal error! The corrupted image has ruined the flash!!");
        goto err;
    }

    D("%s, file_begin=%d, %d\n", user_filename, file_begin, file_end - file_begin);
    printf("<h1>FW Update OK!</h1>");
    printf("rebooting...");

    free(line);
    free(boundary);
    system("reboot &");
    if(!access(filename, F_OK)){
        remove(filename);
    }
    sleep(1);
    remove(lockfile);
    exit(0);
err:
    free(boundary);
    remove(lockfile);
    exit(-1);
}
