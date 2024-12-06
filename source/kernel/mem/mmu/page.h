#ifndef __PAGE_H
#define __PAGE_H
#include "list.h"
#include "types.h"
#include "cpu_cfg.h"
#include "string.h"
#include "mmu.h"
#define PAGE_ALLOW_MAX (MEM_TOTAL_SIZE / MEM_PAGE_SIZE)

typedef struct _vm_node_t
{
    int pressent;
    ph_addr_t vm;
    list_node_t node;
} vm_node_t;
typedef enum _page_type_t
{
    PAGE_TYPE_KENNEL, //初始化所有页全是内核页
    PAGE_TYPE_ANONYM, // 匿名页，通常是动态分配的
    PAGE_TYPE_FILE,   // 文件页，一般是用于mmap或共享内存的
    PAGE_TYPE_CACHE,  // 磁盘缓存页
} page_type_t;
typedef struct _page_t
{

    page_type_t type;
    list_t rela_vm_list;
    int ref;
} page_t;

page_t *page_alloc(ph_addr_t ph, page_type_t type);
void page_free(page_t *page);
vm_node_t *vm_alloc(ph_addr_t vm);
void vm_free(vm_node_t *virtual);
int page_remove_vm_from_list(ph_addr_t vm,page_t* page);
page_t *get_page_struct(ph_addr_t ph);
int page_add_vmnode(page_t* page,ph_addr_t vm);

int page_record_map_ref(ph_addr_t vm,page_t* page);
int page_dispel_map_ref(ph_addr_t vm,pde_t* page_dir);
int page_get_ph_ref(ph_addr_t vm,pde_t* page_dir);
#endif
