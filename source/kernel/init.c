
#include "boot_info.h"
#include "debug.h"
#include "rtc.h"
boot_info_t* boot_inform;
void kernel_init(boot_info_t *boot_info)
{
    rs232_init();
    time_init();
    boot_inform = boot_info;
    dbg_info("hello world\r\n");
    ASSERT(1==2);
    int a = 1;
    while (1)
    {
        ;
    }
    
}
