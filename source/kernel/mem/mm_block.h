#ifndef __MM_BLOCK_H
#define __MM_BLOCK_H

#include "types.h"
#define MEM_PAGE_SIZE 4096
#define MEM_MMREGION_MAX_CNT    1024
#define MEM_RVREGION_MAX_CNT    1024
typedef uint32_t ph_addr_t;
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
}memblock_t;

void memblock_init(void);
int free_pages(ph_addr_t addr,uint32_t n);
ph_addr_t alloc_pages(uint32_t n);
#endif
