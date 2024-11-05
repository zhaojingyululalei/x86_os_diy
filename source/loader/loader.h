#ifndef __LOADER_H
#define __LOADER_H

#include "types.h"
#include "boot_info.h"


#define SYS_KERNEL_LOAD_ADDR (1024*1024)

// 内存检测信息结构
typedef struct SMAP_entry {
    uint32_t BaseL; // base address uint64_t
    uint32_t BaseH;
    uint32_t LengthL; // length uint64_t
    uint32_t LengthH;
    uint32_t Type; // entry Type
    uint32_t ACPI; // extended
}__attribute__((packed)) SMAP_entry_t;

extern boot_info_t boot_info;

#endif
