#ifndef __MM_BLOCK_H
#define __MM_BLOCK_H

#include "types.h"
#include "ipc/mutex.h"
#define MEM_PAGE_SIZE 4096
#define MEM_MMREGION_MAX_CNT    1024
#define MEM_RVREGION_MAX_CNT    1024

typedef struct _memblock_region_t {
    ph_addr_t base;      // 区域的起始物理地址
    ph_addr_t size;      // 区域的大小
    enum {
        NONE=0,
        MEMORY,
        RESERVED,
        
    }flag;
}memblock_region_t;

typedef struct _memblock_type_t {
    uint32_t cnt;            // 当前区域数
    uint32_t max;            // 区域的最大数目
    memblock_region_t *regions; // 指向内存区域数组
}memblock_type_t;

typedef struct _memblock_t {
    memblock_type_t memory;    // 表示可用的内存区域
    memblock_type_t reserved;  // 表示已保留的内存区域

    mutex_t mutex;
}memblock_t;

/*所有操作都是以页为单位，后续实现malloc才不以页为单位*/
void memblock_init(void);
int mm_free_pages(ph_addr_t addr,uint32_t n);
//分配连续的几页内存---物理地址操作
ph_addr_t mm_alloc_pages(uint32_t n);
ph_addr_t mm_alloc_one_page(void);
int mm_free_one_page(ph_addr_t addr);

//虚拟地址操作
//给某个进程的虚拟地址开辟空间
#include "mmu/mmu.h"
int mmblock(task_t* task,ph_addr_t vm_start,uint32_t n);

#endif
