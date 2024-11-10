#include "mm_block.h"
#include "init.h"
#include "mem_addr_cfg.h"
#include "debug.h"
static memblock_region_t mm_regions[MEM_MMREGION_MAX_CNT] = {0};
static memblock_region_t rv_regions[MEM_RVREGION_MAX_CNT] = {0};
static memblock_t memblock;

static memblock_region_t* get_region_from(memblock_type_t* mm)
{
    for (int i = 0; i < mm->max; i++)
    {
        if (mm->regions[i].flag == NONE) {
            return &mm->regions[i];
        }
    }
    return NULL;
}

static int free_region_in(memblock_region_t* region, memblock_type_t* mm)
{
    memblock_region_t* start = mm->regions;
    for (int i = 0; i < mm->max; i++, start++)
    {
        if (start == region) {
            start->flag = NONE;
            return 0;
        }
    }
    return -1;
}

ph_addr_t alloc_pages(uint32_t n) {
    ph_addr_t ret = -1;
    memblock_region_t* region = NULL;
    memblock_region_t* best_fit = NULL;
    uint32_t best_fit_size = UINT32_MAX;

    for (int i = 0; i < MEM_MMREGION_MAX_CNT; i++) {
        if (mm_regions[i].flag == MEMORY) {
            uint32_t region_size = mm_regions[i].size;

            if (region_size >= n * MEM_PAGE_SIZE && region_size < best_fit_size) {
                best_fit = &mm_regions[i];
                best_fit_size = region_size;

                if (region_size == n * MEM_PAGE_SIZE) {
                    break;
                }
            }
        }
    }

    if (best_fit != NULL) {
        ret = best_fit->base;

        if (best_fit_size > n * MEM_PAGE_SIZE) {
            best_fit->base += n * MEM_PAGE_SIZE;
            best_fit->size -= n * MEM_PAGE_SIZE;
        } else {
            best_fit->flag = NONE;
            memblock.memory.cnt--;
        }

        region = get_region_from(&memblock.reserved);
        if (region != NULL) {
            memblock.reserved.cnt++;
            region->flag = RESERVED;
            region->base = ret;
            region->size = n * MEM_PAGE_SIZE;
        } else {
            // 处理错误，避免空指针访问
            dbg_warning("内存不足，无法在 reserved 中记录分配信息\n");
            return -1;
        }
    }

    return ret;
}

int free_pages(ph_addr_t addr, uint32_t n)
{
    memblock_region_t* region;
    for (int i = 0; i < MEM_RVREGION_MAX_CNT; i++) {
        if (rv_regions[i].flag == RESERVED) {
            if (rv_regions[i].base == addr) {
                if (n * MEM_PAGE_SIZE != rv_regions[i].size) {
                    dbg_warning("分配和释放的内存不匹配\n");
                }
                rv_regions[i].flag = NONE;
                memblock.reserved.cnt--;

                region = get_region_from(&memblock.memory);
                if (region != NULL) {
                    memblock.memory.cnt++;
                    region->flag = MEMORY;
                    region->base = addr;
                    region->size = n * MEM_PAGE_SIZE;
                    return 0;
                } else {
                    dbg_warning("无法在 memory 中记录释放的区域\n");
                    return -1;
                }
            }
        }
    }
    return -1;
}

void memblock_init(void)
{
    for (int i = 0; i < MEM_MMREGION_MAX_CNT; i++) {
        mm_regions[i].flag = NONE;
    }
    for (int i = 0; i < MEM_RVREGION_MAX_CNT; i++) {
        rv_regions[i].flag = NONE;
    }

    memblock.memory.cnt = 1;
    memblock.memory.max = MEM_MMREGION_MAX_CNT;
    memblock.memory.regions = mm_regions;

    memblock.reserved.cnt = 0;
    memblock.reserved.max = MEM_RVREGION_MAX_CNT;
    memblock.reserved.regions = rv_regions;

    uint32_t mm_start = KERNEL_START_ADDR_REL + KERNEL_SIZE;
    uint32_t mm_end = boot_inform->ram_region_cfg[1].start + boot_inform->ram_region_cfg[1].size - 1;

    memblock_region_t* region = get_region_from(&memblock.memory);
    if (region != NULL) {
        region->flag = MEMORY;
        region->base = mm_start;
        region->size = mm_end - mm_start + 1;
        memblock.memory.cnt++;
    } else {
        dbg_warning("内存区域初始化失败\n");
    }
}
