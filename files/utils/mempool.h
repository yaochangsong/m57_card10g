#ifndef __MEMPOOL_H_
#define __MEMPOOL_H_

#include <stdbool.h>

#define MEMORY_POOL_ALLOCED_CONST   ((void *) 0xFFFFFFFFu)
typedef struct memory_pool_s
{
  void *pool;           //内存池
  void *empty_blocks;   //空的blocks
  int used_count;
  size_t block_size;    //block的大小
  size_t count;         //block的数量
  void *attr;           //属性参数
  size_t attr_len;      //属性参数长度
  bool is_attr_write;   //属性参数写入标志 true,false
} __attribute__ ((__aligned__)) memory_pool_t;

memory_pool_t * memory_pool_create(size_t bs, size_t c);
void memory_pool_destroy(memory_pool_t *mp);
void memory_pool_clear(memory_pool_t *mp);
void memory_pool_dump(memory_pool_t *mp, void (* print_func) (void *value));
void * memory_pool_alloc(memory_pool_t *mp);
void * memory_pool_alloc_blocksize_num(memory_pool_t *mp, size_t blocksize_num);
void memory_pool_free(memory_pool_t *mp);
void memory_pool_set_pool_step(memory_pool_t *mp, size_t size);
size_t  memory_pool_get_use_count(memory_pool_t *mp);
size_t  memory_pool_get_count(memory_pool_t *mp);
size_t  memory_pool_get_attr_len(memory_pool_t *mp);
void *memory_pool_first(memory_pool_t *mp);



#endif /* __MEMPOOL_H_ */
