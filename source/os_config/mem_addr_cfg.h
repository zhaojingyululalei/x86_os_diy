#ifndef __MEM_ADDR_CFG_H
#define __MEM_ADDR_CFG_H

#define BOOTLOADER_SEG 0x0

#define BOOT_START_ADDR_REL  0x7c00
#define LOAD_START_ADDR_REL  0x8000 
#define KERNEL_START_ADDR_REL   0x100000
#define KERNEL_SIZE (32*1024*1024)

#define STACK_KERNEL_TOP_ADDR   0x7000
#endif