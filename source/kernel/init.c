#include "init.h"
#include "debug.h"
#include "rtc.h"
#include "mem/mm_block.h"
boot_info_t* boot_inform = NULL;

void test_memblock() {
    memblock_init();
    
    dbg_info("初始化完成。分配 2 页（每页 4096 字节）\r\n");
    ph_addr_t addr1 = mm_alloc_pages(2);
    dbg_info("分配的地址：0x%x\r\n", addr1);

    dbg_info("再分配 3 页\r\n");
    ph_addr_t addr2 = mm_alloc_pages(3);
    dbg_info("分配的地址：0x%x\r\n", addr2);

    dbg_info("释放第一个分配的地址：0x%x\r\n", addr1);
    if (mm_free_pages(addr1, 2) == 0) {
        dbg_info("释放成功\r\n");
    } else {
        dbg_info("释放失败\r\n");
    }

    dbg_info("释放第二个分配的地址：0x%x\r\n", addr2);
    if (mm_free_pages(addr2, 3) == 0) {
        dbg_info("释放成功\r\n");
    } else {
        dbg_info("释放失败\r\n");
    }
}
#include "mem/memory.h"
#include "cpu.h"
void kernel_init(boot_info_t *boot_info)
{
    boot_inform = boot_info;
    serial_init();
    time_init();
    memory_init();
    cpu_init();
    irq_enable_global();
    int i = 1 /0;
    while (1)
    {
        ;
    }
    
}
