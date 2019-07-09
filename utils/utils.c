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

