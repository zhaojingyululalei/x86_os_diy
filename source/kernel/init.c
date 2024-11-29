#include "init.h"
#include "debug.h"
#include "rtc.h"
#include "mem/mm_block.h"
#include "timer.h"
boot_info_t *boot_inform = NULL;

#include "mem/memory.h"
#include "cpu.h"
#include "task/task.h"
#include "cpu_instr.h"
task_t second_task;

void func(void)
{
    while (1)
    {

        dbg_info("i am second task\r\n");
        for (int i = 0; i < 0xFFFFFF; i++)
        {
            ;
        }
    }
}

void kernel_init(boot_info_t *boot_info)
{
    boot_inform = boot_info;
    serial_init();
    rtc_init();
    memory_init();
    cpu_init();
    sched_init();
    timer_init();
    irq_enable_global();

    irq_state_t state = irq_enter_protection();
    task_init(&second_task, KERNEL, func, NULL);
    set_task_to_ready_list(&second_task);
    irq_leave_protection(state);

    // int i = 1 /0;
    while (1)
    {
        dbg_info("i am first task\r\n");
        for (int i = 0; i < 0xFFFFFF; i++)
        {
            ;
        }
    }
}
