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
#include "ipc/mutex.h"
#include "ipc/semaphor.h"
task_t second_task;

mutex_t mutex;
sem_t sem;

DEFINE_PROCESS_FUNC(second_func)
{
    sys_sleep_ms(1000);
    while (1)
    {
        sys_sem_notify(&sem);
        dbg_info("product an apple\r\n");
        sys_sleep_ms(1000);
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
    sys_mutex_init(&mutex);
    sys_sem_init(&sem,5);
    create_kernel_process(&second_task,second_func);
    

    // int i = 1 /0;
    while (1)
    {
        sys_sem_wait(&sem);
        dbg_info("i eat an apple\r\n");
        
        
    }
}
