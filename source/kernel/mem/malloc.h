#ifndef __MALLOC_H
#define __MALLOC_H

#include "cpu_cfg.h"
#include "types.h"
#include "list.h"
#define  MAX_FREE_BLOCK_SIZE    1024



/*块节点*/
typedef struct _free_block_t
{
    enum{
        F_BLOCK_TYPE_NONE,
        F_BLOCK_TYPE_CREATE,
        F_BLOCK_TYPE_EFFCTIVE,
        F_BLOCK_TYPE_SPLIT
    }state;
    ph_addr_t start;
    uint32_t capcity;

    int list_idx;
    uint32_t remain_size; //剩余可用容量
    struct _free_block_t* buddy; //伙伴
    struct _free_block_t* parent;

    list_node_t node;
}free_block_t;
/*空闲链表内存分配器 */
typedef struct _flmallocor_t
{
    //0~12
    list_t free_list[HEAP_FREE_LIST_NR];
    int size[HEAP_FREE_LIST_NR];
}flmallocor_t;


/**
 * malloc 信息头
 */
#pragma pack(1)
typedef struct _mlk_info_head_t
{
    uint32_t size;
    free_block_t* block; //这个内存属于哪个节点
}mlk_info_head_t;
#pragma pack()

/* */





void fmlk_debug_list(int idx,flmallocor_t* f);
void fmlk_debug_alllist(flmallocor_t* f);
free_block_t *mlk_alloc_block(void);
void mlk_free_block(free_block_t *block);
int mlk_put_block_into_list(free_block_t* block,int idx,flmallocor_t* f);
int mlk_remove_block_from_list(free_block_t* block,int idx,flmallocor_t* f);
free_block_t* mlk_get_first_block_from_list(int idx,flmallocor_t* f);

void sbrk(uint32_t size,flmallocor_t* f);
void flmallocor_init(flmallocor_t* f);
int mlk_split(free_block_t* block,int size,flmallocor_t* f);
int mlk_collect(free_block_t* block,flmallocor_t* f);
ph_addr_t mlk_alloc(uint32_t size,free_block_t** blk,flmallocor_t* f);

void* sys_malloc(uint32_t size);
void sys_free(void* ptr);

#endif

