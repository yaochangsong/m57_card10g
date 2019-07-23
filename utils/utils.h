#ifndef _UTILS_H_H_
#define _UTILS_H_H_


/*向上取整*/
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d)) 
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))


char *safe_strdup(const char *s);

#endif

