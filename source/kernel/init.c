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
#include "net/net.h"
#include "console.h"
#include "kbd.h"
#include "mouse.h"
#include "dev.h"
task_t thello;
task_t tworld;
sem_t sem;

mutex_t mutex;
DEFINE_PROCESS_FUNC(hello)
{
    int timeout = 5000,ret = 0;
    tm_t time, tmo_time;
    time_t now_sec, tmo_sec;
    // 获取当前时间
    sys_get_clocktime(&time);
    now_sec = sys_mktime(&time);
    // 设置绝对超时时间

    uint32_t diff = (timeout + (999)) / 1000; // ms--->s 向上去整
    tmo_sec = now_sec + diff;
    sys_local_time(&tmo_time, tmo_sec);
    ret = sys_sem_timedwait(&sem, &tmo_time); 
    dbg_info("ret = %d\r\n",ret);
    
    while (1)
    {
        sys_sleep_ms(1000);
    }

    return NULL;
}
DEFINE_PROCESS_FUNC(world)
{
    sys_sleep_ms(500);
   // sys_sem_notify(&sem);
    while (1)
    {
        sys_sleep_ms(1000);
    }
    return NULL;
}
task_t cp_first_task;
void jmp_to_first_task(void)
{
    
    task_t* first_task = &cp_first_task;
    tss_t *tss = &(first_task->tss);

    // 也可以使用类似boot跳loader中的函数指针跳转
    // 这里用jmp是因为后续需要使用内联汇编添加其它代码
    __asm__ __volatile__(
        // 模拟中断返回，切换入第1个可运行应用进程
        // 不过这里并不直接进入到进程的入口，而是先设置好段寄存器，再跳过去
        "push %[ss]\n\t"     // SS
        "push %[esp]\n\t"    // ESP
        "push %[eflags]\n\t" // EFLAGS
        "push %[cs]\n\t"     // CS
        "push %[eip]\n\t"    // ip
        "iret\n\t" ::[ss] "r"(tss->ss),
        [esp] "r"(tss->esp), [eflags] "r"(tss->eflags),
        [cs] "r"(tss->cs), [eip] "r"(tss->eip));
}

// #include "dev/chr/rtl8139.h"
// static rtl8139_priv_t priv;

extern void net_test(void);

void kernel_init(boot_info_t *boot_info)
{
    boot_inform = boot_info;
    serial_init();
    rtc_init();
    task_ipc_init();
    memory_init();
    cpu_init();
    fs_init();
    // int dev_id = dev_open(DEV_TTY,0,NULL);
    // dev_write(dev_id,0,"hello\r\n",7);
    kbd_init(); 
    mouse_init();
    
    sched_init();
    timer_init();
    
    // sys_sem_init(&sem,0);
    // sys_mutex_init(&mutex);
     //create_kernel_process(&thello, hello);
    // create_kernel_process(&tworld, world);

    first_task_init();
    task_t *first_task = get_cur_task();
    ASSERT(first_task != 0);
    cp_first_task = *first_task;
    //net_init();
    irq_enable_global();
    
    //net_test();
    // while (1)
    // {
    //     //dbg_info("eat an apple\r\n");
    //     sys_sleep_ms(1000);
    // }

    jmp_to_first_task();
    while (1)
    {
        //如果执行到这里就不对
        dbg_info("init wrong\r\n");
        sys_sleep_ms(1000);
    }
}
