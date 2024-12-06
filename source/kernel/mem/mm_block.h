#ifndef __MM_BLOCK_H
#define __MM_BLOCK_H

#include "types.h"
#include "ipc/mutex.h"
#include "mmu/page.h"
#include "list.h"
//#define REGION_SIZE_INIT    (4*1024)
#define MEM_REGION_MAX_CNT 2048
typedef struct _memblock_region_t {
    ph_addr_t base;      // 区域的起始物理地址
    uint32_t size;      // 区域的大小
    enum {
        REGION_TYPE_NONE=0,
        REGION_TYPE_MEMORY,
        REGION_TYPE_RESERVED,
        
    }flag;
    list_node_t node;
}memblock_region_t;

typedef struct _memblock_type_t {
    uint32_t cnt;            // 当前区域数
    uint32_t max;            // 区域的最大数目
    list_t regions; // 指向内存区域数组
}memblock_type_t;

typedef struct _memblock_t {
    memblock_type_t memory;    // 表示正在使用的内存区域
    memblock_type_t reserved;  // 表示未使用，待分配的内存区域

    mutex_t mutex;
}memblock_t;



/*只要操作内核区，用这些api*/
/*所有操作都是以页为单位，后续实现malloc才不以页为单位*/
void memblock_init(void);
int mm_free_pages(ph_addr_t addr,uint32_t n);
ph_addr_t mm_alloc_pages(uint32_t n);
ph_addr_t mm_alloc_one_page(void);
int mm_free_one_page(ph_addr_t addr);

/*操作虚拟地址，用这些api*/
//给某个进程的虚拟地址开辟空间，操作的都是任务自己的页表，互不干扰，不上锁
#include "mmu/mmu.h"
int mmblock(task_t* task,ph_addr_t vm_start,uint32_t n);
int mmfree(task_t* task,ph_addr_t vm_start,uint32_t n);


#endif
