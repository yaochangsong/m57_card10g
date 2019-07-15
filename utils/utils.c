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
#include "config.h"

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


int get_mac(char * mac, int len_limit)    
{
    struct ifreq ifreq;
    int sock;

    if(mac ==NULL)
        return -1;
    
    if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror ("socket");
        return -1;
    }
    strcpy (ifreq.ifr_name, NETWORK_EHTHERNET_POINT);    

    if (ioctl (sock, SIOCGIFHWADDR, &ifreq) < 0)
    {
        perror ("ioctl");
        return -1;
    }
    
    memcpy(mac, &ifreq.ifr_hwaddr.sa_data, len_limit);
    
    return 0;
}


#define CRC_16_POLYNOMIALS   0x8005
uint16_t crc16_caculate(uint8_t *pchMsg, uint16_t wDataLen) {
    uint8_t i;
    uint8_t chChar;
    uint16_t wCRC = 0xFFFF; //g_CRC_value;

    while (wDataLen--) {
        chChar = *pchMsg++;
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

