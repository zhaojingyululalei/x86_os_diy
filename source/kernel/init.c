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
task_t kernel_task;

DEFINE_PROCESS_FUNC(second_func)
{
    //sbrk(4098);
    //int x= mlk_alloc(218);
    //fmlk_debug_alllist();
    //free_block_t *fblk = mlk_get_first_block_from_list(12);
    //mlk_split(fblk,14);
    //fmlk_debug_alllist();
    //mlk_collect(mlk_get_first_block_from_list(4));
    char* p = sys_malloc(259);
    fmlk_debug_alllist(&(get_cur_task()->flmlk));
    sys_free(p);
    fmlk_debug_alllist(&(get_cur_task()->flmlk));

    while (1)
    {

        dbg_info("product an apple\r\n");
        sys_sleep_ms(1000);
    }
}

void jmp_to_first_task(void)
{
    task_t *curr = get_cur_task();
    ASSERT(curr != 0);

    tss_t *tss = &(curr->tss);

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
void kernel_init(boot_info_t *boot_info)
{
    boot_inform = boot_info;
    serial_init();
    rtc_init();
    memory_init();
    cpu_init();
    sched_init();
    timer_init();

    create_kernel_process(&second_task, second_func);

    first_task_init();
    irq_enable_global();
    jmp_to_first_task();
    while (1)
    {
        dbg_info("eat an apple\r\n");
        sys_sleep_ms(1000);
    }
}
