#include "mm_block.h"
#include "init.h"
#include "mem_addr_cfg.h"
#include "debug.h"
#include "math.h"
#include "list.h"

/*分配的页有大部分用在进程拷贝内核页表上*/
static memblock_region_t all_regions[MEM_REGION_MAX_CNT] = {0};
static memblock_t g_memblock;
static void region_debug_print(void)
{
    memblock_t *memblock = &g_memblock;

    // 打印 memory 区域
    dbg_info("Memory Regions:\r\n");
    dbg_info("Total Count: %d\r\n", memblock->memory.cnt);
    memblock_region_t *cur_mem = list_node_parent(memblock->memory.regions.first, memblock_region_t, node);

    while (cur_mem)
    {
        dbg_info("  [Memory] Base: 0x%x, Size: 0x%x\r\n", cur_mem->base, cur_mem->size);
        cur_mem = list_node_parent(list_node_next(&cur_mem->node), memblock_region_t, node);
    }

    // 打印 reserved 区域
    dbg_info("Reserved Regions:\r\n");
    dbg_info("Total Count: %d\r\n", memblock->reserved.cnt);
    memblock_region_t *cur_rsv = list_node_parent(memblock->reserved.regions.first, memblock_region_t, node);

    while (cur_rsv)
    {
        dbg_info("  [Reserved] Base: 0x%x, Size: 0x%x\r\n", cur_rsv->base, cur_rsv->size);
        cur_rsv = list_node_parent(list_node_next(&cur_rsv->node), memblock_region_t, node);
    }
}
void memblock_init(void)
{
    uint32_t mm_start = KERNEL_START_ADDR_REL + KERNEL_SIZE;

    memblock_t *memblock = &g_memblock;
    sys_mutex_init(&memblock->mutex);

    list_init(&memblock->memory.regions);
    list_init(&memblock->reserved.regions);

    memblock->memory.cnt = 0;
    memblock->reserved.cnt = 0;

    // 初始化一个大的 reserved 区域
    all_regions[0].base = mm_start;
    all_regions[0].size = MEM_TOTAL_SIZE - mm_start - MEM_PAGE_SIZE;
    all_regions[0].flag = REGION_TYPE_RESERVED;

    list_insert_last(&memblock->reserved.regions, &all_regions[0].node);
    memblock->reserved.cnt++; // 初始只有一个 reserved 区域
}

ph_addr_t mm_alloc_pages(uint32_t n)
{
    if (n == 0)
    {
        dbg_error("Invalid allocation request: 0 pages.\r\n");
        return 0;
    }

    uint32_t alloc_size = n * MEM_PAGE_SIZE;
    memblock_t *memblock = &g_memblock;
    list_node_t *node;
    memblock_region_t *region;

    sys_mutex_lock(&memblock->mutex);

    node = memblock->reserved.regions.first;
    while (node)
    {
        region = list_node_parent(node, memblock_region_t, node);

        if (region->size >= alloc_size)
        {
            // 分配区域
            ph_addr_t alloc_base = region->base;

            region->base += alloc_size;
            region->size -= alloc_size;

            // 如果分配后区域为 0，移除该区域
            if (region->size == 0)
            {
                list_remove(&memblock->reserved.regions, &region->node);
                memblock->reserved.cnt--;
                region->flag = REGION_TYPE_NONE; // 置为未使用
            }

            // 创建新的 memory 区域
            for (int i = 0; i < MEM_REGION_MAX_CNT; i++)
            {
                if (all_regions[i].flag == REGION_TYPE_NONE)
                {
                    all_regions[i].base = alloc_base;
                    all_regions[i].size = alloc_size;
                    all_regions[i].flag = REGION_TYPE_MEMORY;

                    list_insert_last(&memblock->memory.regions, &all_regions[i].node);
                    memblock->memory.cnt++;
                    break;
                }
            }

            sys_mutex_unlock(&memblock->mutex);
            return alloc_base;
        }

        node = node->next;
    }

    sys_mutex_unlock(&memblock->mutex);
    dbg_error("Allocation failed: not enough memory.\r\n");
    return 0;
}

int mm_free_pages(ph_addr_t addr, uint32_t n) {
    if (addr == 0 || n == 0) {
        dbg_error("Invalid free request: addr=0x%x, n=%u\r\n", addr, n);
        return -1;
    }

    uint32_t free_size = n * MEM_PAGE_SIZE;
    memblock_t *memblock = &g_memblock;
    list_node_t *node;
    memblock_region_t *region;

    sys_mutex_lock(&memblock->mutex);

    node = memblock->memory.regions.first;
    while (node) {
        region = list_node_parent(node, memblock_region_t, node);

        if (region->base <= addr && addr + free_size <= region->base + region->size) {
            // 如果完全匹配，移除该区域
            if (region->base == addr && region->size == free_size) {
                list_remove(&memblock->memory.regions, &region->node);
                memblock->memory.cnt--;
                region->flag = REGION_TYPE_NONE;

                // 将释放的区域加入 reserved 区域
                for (int i = 0; i < MEM_REGION_MAX_CNT; i++) {
                    if (all_regions[i].flag == REGION_TYPE_NONE) {
                        all_regions[i].base = addr;
                        all_regions[i].size = free_size;
                        all_regions[i].flag = REGION_TYPE_RESERVED;

                        list_insert_last(&memblock->reserved.regions, &all_regions[i].node);
                        memblock->reserved.cnt++;
                        break;
                    }
                }

                // 合并相邻的 reserved 区域
                memblock_region_t *cur_rsv = list_node_parent(memblock->reserved.regions.first, memblock_region_t, node);
                while (cur_rsv) {
                    memblock_region_t *next_rsv = list_node_parent(list_node_next(&cur_rsv->node), memblock_region_t, node);

                    if (next_rsv && cur_rsv->base + cur_rsv->size == next_rsv->base) {
                        cur_rsv->size += next_rsv->size;

                        list_remove(&memblock->reserved.regions, &next_rsv->node);
                        memblock->reserved.cnt--;
                        next_rsv->flag = REGION_TYPE_NONE;
                    } else {
                        cur_rsv = next_rsv;
                    }
                }

                sys_mutex_unlock(&memblock->mutex);
                return 0;
            }

            // 如果是部分释放，需要拆分区域
            if (region->base == addr) {
                // 释放区域位于开头
                region->base += free_size;
                region->size -= free_size;
            } else if (addr + free_size == region->base + region->size) {
                // 释放区域位于末尾
                region->size -= free_size;
            } else {
                // 释放区域位于中间，需要拆分区域
                ph_addr_t new_base = addr + free_size;
                uint32_t new_size = region->base + region->size - new_base;

                // 更新原区域为左侧部分
                region->size = addr - region->base;

                // 创建新的右侧部分区域
                for (int i = 0; i < MEM_REGION_MAX_CNT; i++) {
                    if (all_regions[i].flag == REGION_TYPE_NONE) {
                        all_regions[i].base = new_base;
                        all_regions[i].size = new_size;
                        all_regions[i].flag = REGION_TYPE_MEMORY;

                        list_insert_behind(&memblock->memory.regions, &region->node, &all_regions[i].node);
                        memblock->memory.cnt++;
                        break;
                    }
                }
            }

            // 将释放的区域加入 reserved 区域
            for (int i = 0; i < MEM_REGION_MAX_CNT; i++) {
                if (all_regions[i].flag == REGION_TYPE_NONE) {
                    all_regions[i].base = addr;
                    all_regions[i].size = free_size;
                    all_regions[i].flag = REGION_TYPE_RESERVED;

                    list_insert_last(&memblock->reserved.regions, &all_regions[i].node);
                    memblock->reserved.cnt++;
                    break;
                }
            }

            // 合并相邻的 reserved 区域
            memblock_region_t *cur_rsv = list_node_parent(memblock->reserved.regions.first, memblock_region_t, node);
            while (cur_rsv) {
                memblock_region_t *next_rsv = list_node_parent(list_node_next(&cur_rsv->node), memblock_region_t, node);

                if (next_rsv && cur_rsv->base + cur_rsv->size == next_rsv->base) {
                    cur_rsv->size += next_rsv->size;

                    list_remove(&memblock->reserved.regions, &next_rsv->node);
                    memblock->reserved.cnt--;
                    next_rsv->flag = REGION_TYPE_NONE;
                } else {
                    cur_rsv = next_rsv;
                }
            }

            sys_mutex_unlock(&memblock->mutex);
            return 0;
        }

        node = node->next;
    }

    sys_mutex_unlock(&memblock->mutex);
    dbg_error("Freeing failed: address 0x%x not found.\r\n", addr);
    region_debug_print();
    return -1;
}


ph_addr_t mm_alloc_one_page(void)
{
    return mm_alloc_pages(1);
}

int mm_free_one_page(ph_addr_t addr)
{
    return mm_free_pages(addr, 1);
}



/**
 * 给进程虚拟空间分配内存
 */
int mmblock(task_t *task, ph_addr_t vm_start, uint32_t n)
{
    // 获取进程自己的页目录
    ph_addr_t page_dir = task_get_page_dir(task);
    if (page_dir == NULL)
    {
        dbg_error("task cr3 not init yet\r\n");
        return -1;
    }
    pde_t *pde = (pde_t *)page_dir;
    // 为进程分配n页物理内存
    ph_addr_t ph_start = mm_alloc_pages(n);
    if (ph_start == NULL)
    {
        return -1;
    }
    for (int i = 0; i < n; i++)
    {
        // 建立虚拟内存和物理内存的映射关系到页表中
        mmu_memory_map((pde_t *)page_dir, vm_start, ph_start, 1, 1);
        vm_start += MEM_PAGE_SIZE;
        ph_start += MEM_PAGE_SIZE;
    }

    return 0;
}

int mmfree(task_t *task, ph_addr_t vm_start, uint32_t n)
{
    int ret = 0;
    ph_addr_t vm = vm_start;
    // 找物理页表
    ph_addr_t page_dir = task_get_page_dir(task);
    if (page_dir == NULL)
    {
        dbg_error("task cr3 not init yet\r\n");
        return -1;
    }
    // 获取虚拟地址对应的pte项
    pte_t *pte_free = mmu_from_vm_get_pte(page_dir, vm_start);
    if (pte_free == NULL)
    {
        dbg_error("The page table corresponding to the virtual address does not exist\r\n");
        return -1;
    }
    for (int i = 0; i < n; i++)
    {
        if (pte_free->present)
        {
            // 减少引用计数
            ret = page_dispel_map_ref(vm, page_dir);
            if (ret < 0)
            {
                dbg_error("mem free failed\r\n");
                return -1;
            }
            // 删除物理内存
            int ref = page_get_ph_ref(vm, page_dir);
            if (ref > 0) // 有其他虚拟内存指向该物理页，不能删
            {
                dbg_warning("还有其他虚拟内存使用该物理页\r\n");
                return -1;
            }
            ret = mm_free_one_page(pte_free->phy_page_addr << 12);
            if (ret < 0)
            {
                dbg_error("mem free failed\r\n");
                return -1;
            }
            pte_free->v = 0;
            pte_free++; // 这都是连续的内存页
            vm += MEM_PAGE_SIZE;
        }
    }
    return 0;
}