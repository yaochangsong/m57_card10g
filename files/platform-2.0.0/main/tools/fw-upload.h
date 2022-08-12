#ifndef __UPLOAD_CGI_H__
#define __UPLOAD_CGI_H__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <wait.h>

#define DEBUG
#ifdef DEBUG
#define D(...) fprintf(stderr, __VA_ARGS__)
#else
#define D(...)
#endif


/*
 *  Uboot image header format
 *  (ripped from mkimage.c/image.h)
 */
#define IH_MAGIC    0x27051956
#define IH_NMLEN    32
typedef struct image_header {
    uint32_t    ih_magic;   /* Image Header Magic Number    */
    uint32_t    ih_hcrc;    /* Image Header CRC Checksum    */
    uint32_t    ih_time;    /* Image Creation Timestamp */
    uint32_t    ih_size;    /* Image Data Size      */
    uint32_t    ih_load;    /* Data  Load  Address      */
    uint32_t    ih_ep;      /* Entry Point Address      */
    uint32_t    ih_dcrc;    /* Image Data CRC Checksum  */
    uint8_t     ih_os;      /* Operating System     */
    uint8_t     ih_arch;    /* CPU architecture     */
    uint8_t     ih_type;    /* Image Type           */
    uint8_t     ih_comp;    /* Compression Type     */
    uint8_t     ih_name[IH_NMLEN];  /* Image Name       */
} image_header_t;

#define BOOT_ARM_VEC_TAB    0xEAFFFFFE
#define WIDTH_DECT_WORD     0xAA995566
#define HEADER_SIGN         0x584c4e58
typedef struct boot_header {
    uint32_t arm_vector_table[8];       /* 0xEAFFFFFE offset: 0x00~0x1f */
    uint32_t wdw;                       /*Width Detection Word 0xAA995566 offset: 020 */
    uint32_t header_sign;               /*  ‘X’,’N’,’L’,’X’  0x584c4e58 offset: 024 */
    uint32_t key_source;                /* 0x00000000: Not Encrypted offset: 0x28 */
    uint32_t header_version;            /* 0x01010000   offset: 0x2c */
    uint32_t source_offset;             /*  offset: 0x30 */
    uint32_t fsbl_image_len;            /*  offset: 0x34 */
}boot_header_t;

#define JFFS2_BITMASK    0x8519
typedef struct jffs2_header {
    uint16_t magic_bitmask;             /* 0x19 0x85 */
}jffs2_header_t;


struct image_part_type {
    char *filename;
    char *mtdname;
};


static inline unsigned int getMTDPartSize(char *part)
{
    char buf[128], name[32], size[32], dev[32], erase[32];
    unsigned int result=0;
    FILE *fp = fopen("/proc/mtd", "r");
    if(!fp){
        fprintf(stderr, "mtd support not enable?");
        return 0;
    }
    while(fgets(buf, sizeof(buf), fp)){
        sscanf(buf, "%s %s %s %s", dev, size, erase, name);
        if(!strcmp(name, part)){
            result = strtol(size, NULL, 16);
            break;
        }
    }
    fclose(fp);
    return result;
}

/*
 *  ps. callee must free memory...
 */
static inline char  *getMTDPartDev(char *part)
{
    char buf[128], name[32], size[32], dev[32], erase[32];
    char *devname= NULL;
    FILE *fp = fopen("/proc/mtd", "r");
    if(!fp){
        fprintf(stderr, "mtd support not enable?");
        return 0;
    }
    while(fgets(buf, sizeof(buf), fp)){
        sscanf(buf, "%s %s %s %s", dev, size, erase, name);
        if(!strcmp(name, part)){
            devname = strdup(dev);
            if(devname[4] == ':')
                devname[4] = 0;
            break;
        }
    }
    fclose(fp);
    return devname;
}


static inline int mtd_write_firmware(char *filename, int offset, int len)
{
  //  char cmd[512];
   // int status;
    //mtd_debug erase /dev/mtd4 0 0x02f00000

    // snprintf(cmd, sizeof(cmd), "/bin/flash -f 0x400000 -l 0x40ffff");
    //system(cmd);
    return 0;
}

static inline int mtd_write_jffs2(char *filename, int offset, int len)
{
    char cmd[512], *devname = NULL;
   // int status;
    unsigned int size;
    
    size = getMTDPartSize("\"jffs2\"");
    devname = getMTDPartDev("\"jffs2\"");
    
    D("====>size:%u, 0x%x,devname:%s\n", size,size, devname);
    //mtd_debug erase /dev/mtd3 0 0x02f00000
    //dd if=rootfs.jffs2 of=/dev/mtdblock3
    if(devname == NULL)
        return -1;
    
    snprintf(cmd, sizeof(cmd), "mtd_debug erase /dev/%s 0 0x%x", devname, size);
    D("%s\n", cmd);
   // status = system(cmd);
   // if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
    //    return -1;
    
    snprintf(cmd, sizeof(cmd), "dd if=%s of=/dev/mtdblock3", filename);
    D("%s\n", cmd);
    //status = system(cmd);
   // if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
   //     return -1;
    sleep(4);
    if(devname)
        free(devname);
    return 0;
}

static inline int mtd_write_part(char *filename, char*partname)
{
    char cmd[512], *devname = NULL;
    int status;
    char tmp[8];
    unsigned int size;
    
    size = getMTDPartSize(partname);
    devname = getMTDPartDev(partname);
    if(devname == NULL || size == 0)
        return -1;
    
    D("====>size:%u, 0x%x,devname:%s\n", size,size, devname);
    sscanf(devname, "%c%c%c%c", &tmp[0], &tmp[1],&tmp[2],&tmp[3]);
    D("num=%d\n", tmp[3]);
    //mtd_debug erase /dev/mtd3 0 0x02f00000
    //dd if=rootfs.jffs2 of=/dev/mtdblock3
    snprintf(cmd, sizeof(cmd), "mtd_debug erase /dev/%s 0 0x%x", devname, size);
    D("%s\n", cmd);
    status = system(cmd);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return -1;
    
    snprintf(cmd, sizeof(cmd), "dd if=%s of=/dev/mtdblock%d ", filename, tmp[3]);
    D("%s\n", cmd);
    status = system(cmd);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return -1;

    if(devname)
        free(devname);
    return 0;
}


#endif

