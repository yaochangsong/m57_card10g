#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "mempool.h"

/*
 *  设置内存池对象信息，比如分配内存池，设置内存池的block大小和数量
 *  然后分配一段block大小*数量的一段内存，然后使用链表将这些block串联起来
 */
memory_pool_t * memory_pool_create(size_t bs, size_t c)
{
  memory_pool_t *mp = malloc(sizeof(memory_pool_t));
  if (!mp)
    return NULL;

  mp->block_size = bs;
  mp->count = c;
  //分配了mp->count个block，
  mp->pool = malloc((mp->block_size) * mp->count);
  memset(mp->pool, 0, (mp->block_size) * mp->count);

  mp->empty_blocks = mp->pool;
  mp->used_count = 0;
  mp->attr_len = 0;
  mp->attr = NULL;
  mp->is_attr_write = false;
  
  return mp;
}

void memory_pool_destroy(memory_pool_t *mp)
{
  if (!mp)
    return;
  if(mp->attr)
      free(mp->attr);
  free(mp->pool);
  free(mp);
}



void memory_pool_dump(memory_pool_t *mp, void (* print_func) (void *value))
{
  printf("start: %p, count: %u, used_count: %d\n", mp->pool,mp->count, mp->used_count);

  void *block;
  size_t i;
  int count = mp->used_count;
  
  for (i = 0; i < mp->count; i++)
  {
    block = (void *) ((uint8_t *) mp->pool + (mp->block_size * i));
    printf("block #%i(%p):", i, block);

    if(count-- > 0){
         printf(" allocated");
         if (print_func)
         {
            printf(", value: ");
            print_func(block);
         }
         printf("\n");
    }else{
         //printf("no value\n");
    }
  }
  printf("\n");
}

/*
 *  从内存池对象中分配一个block
 *
 */
void * memory_pool_alloc(memory_pool_t *mp)
{
  void *p;
  if(mp->used_count < mp->count){
    p = mp->empty_blocks;
    mp->empty_blocks = (void*)((uint8_t *) mp->empty_blocks + mp->block_size);
    mp->used_count ++;
    return p;
  }else{
    return NULL;
  }
}

void memory_pool_write_value(void *w_addr, void *data, size_t data_byte_len)
{
    if(w_addr && data)
        memcpy(w_addr, data, data_byte_len);
}

void memory_pool_write_attr_value(memory_pool_t *mp, void *attr, size_t attr_byte_len)
{
    if(mp && attr && (mp->is_attr_write == false)){
        if(mp->attr != NULL){
            free(mp->attr);
            mp->attr = NULL;
        }
        mp->attr = malloc(attr_byte_len);
        memcpy(mp->attr, attr, attr_byte_len);
        mp->attr_len = attr_byte_len;
        mp->is_attr_write = true;
    }
}

void *memory_pool_get_attr(memory_pool_t *mp)
{
    return mp->attr;
}


size_t  memory_pool_get_attr_len(memory_pool_t *mp)
{
    return mp->attr_len;
}


size_t  memory_pool_get_count(memory_pool_t *mp)
{
    return mp->count;
}

size_t  memory_pool_get_use_count(memory_pool_t *mp)
{
    return mp->used_count;
}

void memory_pool_set_pool_step(memory_pool_t *mp, size_t size)
{
    mp->block_size = size;
}


void memory_pool_read_int16_value(void *r_addr, void *data, size_t data_byte_len)
{
    int *p;
    for(int i =0; i < data_byte_len/sizeof(int16_t); i++){
        p = (int *) r_addr+i;
        printf("%d ", i);
        printf("%p ", p);
        printf("%d ", *p);
        printf("\n");
    }
}

void *memory_pool_first(memory_pool_t *mp)
{
    return mp->pool;
}

size_t memory_pool_step(memory_pool_t *mp)
{
    return mp->block_size;
}

void *memory_pool_end(memory_pool_t *mp)
{
    return (mp->pool + mp->used_count*mp->block_size);
}


/*
 *  回收block
 *
 */
void memory_pool_free(memory_pool_t *mp)
{
    if(mp && mp->pool){
        mp->used_count = 0;
        memset(mp->pool, 0, (mp->block_size) * mp->count);
        mp->empty_blocks = mp->pool;
        mp->is_attr_write = false;
        mp->attr_len = 0;
        if(mp->attr){
            free(mp->attr);
            mp->attr = NULL;
        }
    }
}

