#ifndef __BOOT_INFO_H
#define __BOOT_INFO_H

#include "types.h"
#define BOOT_RAM_REGION_MAX			10
/**
 * 启动信息参数
 */
#pragma pack(1)
typedef struct _boot_info_t {
    uint32_t gdt_base_addr;
    uint32_t gdt_entry_nr;
    uint32_t gdt_entry_limit;
    uint32_t idt_base_addr;
    uint32_t idt_entry_nr;
    uint32_t idt_entry_limit;
    struct {
        uint32_t start;
        uint32_t size;
    }ram_region_cfg[BOOT_RAM_REGION_MAX];
    int ram_region_count;
}boot_info_t;

#pragma pack()

#endif
