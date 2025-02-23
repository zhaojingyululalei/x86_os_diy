

#include "task.h"
#include "debug.h"
#include "cpu.h"
#include "cpu_instr.h"
#include "mem/memory.h"
#include "string.h"
static schedulor_t schedulor;
// static uint8_t idle_stack[1024];
static void idle_func(void)
{
    while (1)
    {
        hlt();
    }
}
static idle_task_init(void)
{
    task_init(&schedulor.idle_task, KERNEL, idle_func, NULL, NULL);
}
void first_task_init(void)
{
    /*链接脚本中的变量*/
    extern void first_task_entry(void);
    extern uint8_t s_first_task_ph, s_first_task_vm, e_first_task_ph, e_first_task_vm;
    ph_addr_t f_task_start_ph = &s_first_task_ph;
    ph_addr_t f_task_start_vm = &s_first_task_vm;
    ph_addr_t f_task_end_ph = &e_first_task_ph;
    ph_addr_t f_task_end_vm = &e_first_task_vm;
    uint32_t code_page_count = (f_task_end_ph - f_task_start_ph + MEM_PAGE_SIZE - 1) / MEM_PAGE_SIZE;
    ph_addr_t first_start = (ph_addr_t)first_task_entry;
    task_init(&schedulor.first_task, USR, first_start, NULL, NULL);
    // 分配进程空间
    mmblock(&schedulor.first_task, first_start, code_page_count);
    // 将用户部分代码拷贝过去
    write_cr3(schedulor.first_task.tss.cr3);
    memcpy(first_start, f_task_start_ph, (uint32_t)(f_task_end_ph - f_task_start_ph));
    set_cur_task(&schedulor.first_task);
    write_tr(schedulor.first_task.tss_sel);
}
void sched_init(void)
{
    pidalloc_init(&pidallocter);

    list_init(&schedulor.ready_list);
    list_init(&schedulor.sleep_list);
    list_init(&schedulor.all_list);
    schedulor.cur_task = NULL;
    idle_task_init();
}

task_t *get_cur_task(void)
{
    return schedulor.cur_task;
}
void set_cur_task(task_t *task)
{
    if (task)
    {
        task->state = TASK_RUNNING;
    }
    schedulor.cur_task = task;
}

task_t *get_ready_task(void)
{
    for (int i = 0; i < (&schedulor.ready_list)->count; ++i)
    {
        list_node_t *fnode = list_remove_first(&schedulor.ready_list);
        if (fnode == NULL)
        {
            return NULL;
        }
        task_t *ftask = list_node_parent(fnode, task_t, node);
        if (ftask->state == TASK_READY)
        {
            return ftask;
        }
        else
        {
            list_insert_last(&schedulor.ready_list, fnode);
        }
    }
    return NULL;
}
void set_task_to_ready_list(task_t *task)
{
    if (!task)
    {
        dbg_error("task is empty\r\n");
        return;
    }
    task->state = TASK_READY;
    list_insert_last(&schedulor.ready_list, &task->node);
}
void set_task_to_all_list(task_t *task)
{
    if (!task)
    {
        dbg_error("task is empty\r\n");
        return;
    }
    task->state = TASK_CREATED;
    list_insert_last(&schedulor.all_list, &task->all_node);
}
void remove_task_from_ready_list(task_t *task)
{
    if (!task)
    {
        dbg_error("task is empty\r\n");
        return;
    }
    list_remove(&schedulor.ready_list, &task->node);
}
void remove_task_from_all_list(task_t *task)
{
    if (!task)
    {
        dbg_error("task is empty\r\n");
        return;
    }
    task->state = TASK_NONE;
    list_remove(&schedulor.all_list, &task->all_node);
}
/**
 * 切换至TSS，即跳转实现任务切换
 */
void switch_to_tss(uint32_t tss_selector)
{
    far_jump(tss_selector, 0);
}
/**
 * 切换任务
 **/
void task_switch_from_to(task_t *from, task_t *to)
{
    if (from == to)
    {
        return;
    }
    switch_to_tss(to->tss_sel);
}
void schedul(void)
{

    task_t *pre_task = get_cur_task();
    task_t *cur_task = get_ready_task();
    // 调度任务都存在，且一样，那就不用调度，tss无法切换相同任务，报错
    if (pre_task == cur_task && pre_task && cur_task)
    {
        return;
    }
    // 没有其他任务准备好
    if (cur_task == NULL)
    {
        // 只要当前任务存在(无论是空闲任务还是有效任务)，那么继续执行当前任务，不用切换
        if (pre_task)
        {
            return;
        }
        // 如果当前任务为空，也就是当前任务被调度到其他地方了。这时再执行空闲任务
        cur_task = &schedulor.idle_task;
    }
    set_cur_task(cur_task);
    // 空闲任务或者task==null，不要往就绪队列里塞
    if (pre_task != &schedulor.idle_task && pre_task != NULL)
    {

        set_task_to_ready_list(pre_task);
    }
    task_switch_from_to(pre_task, cur_task);
}

void task_time_tick(void)
{
    irq_state_t state = irq_enter_protection();

    // 睡眠处理
    list_node_t *sleep_node = list_first(&schedulor.sleep_list);
    while (sleep_node)
    {
        list_node_t *next = list_node_next(sleep_node);
        task_t *task = list_node_parent(sleep_node, task_t, node);
        if (--task->sleep_ticks == 0)
        {
            task_wakeup(task);
            set_task_to_ready_list(task);
        }
        sleep_node = next;
    }

    // 时间片处理
    task_t *cur_task = get_cur_task();
    if (cur_task == NULL)
    {
        set_cur_task(&schedulor.idle_task);
        switch_to_tss(schedulor.idle_task.tss_sel);
    }
    else
    {
        if (--cur_task->slice_ticks == 0)
        {
            cur_task->slice_ticks = cur_task->attr.time_slice;
            schedul();
        }
    }

    irq_leave_protection(state);
}

void task_goto_sleep(task_t *task)
{
    if (!task)
    {
        dbg_warning("task is NULL");
        return;
    }
    task->state = TASK_SLEEP;
    list_insert_last(&schedulor.sleep_list, &task->node);
}

void task_wakeup(task_t *task)
{
    list_remove(&schedulor.sleep_list, &task->node);
}
#include "dev/timer.h"
/**
 * 至少睡10ms
 */
int sys_sleep_ms(int time)
{
    irq_state_t state = irq_enter_protection();
    if (time < OS_TICK_MS)
    {
        time = OS_TICK_MS;
    }
    task_t *cur_task = get_cur_task();
    if (!cur_task)
    {
        return -1;
    }
    cur_task->sleep_ticks = (time + (OS_TICK_MS - 1)) / OS_TICK_MS;
    set_cur_task(NULL);
    //remove_task_from_ready_list(cur_task);
    task_goto_sleep(cur_task);
    schedul();
    irq_leave_protection(state);
    return 0;
}

task_t *task_all_list_get_first()
{
    list_node_t *fnode = list_first(&schedulor.all_list);
    task_t *ftask = list_node_parent(fnode, task_t, all_node);
    return ftask;
}

task_t *task_all_list_get_next(task_t *cur)
{
    list_node_t *cnode = &cur->all_node;
    list_node_t *nnode = list_node_next(cnode);
    if (!nnode)
    {
        return NULL;
    }
    task_t *next = list_node_parent(nnode, task_t, all_node);
    return next;
}
task_t *get_init_task()
{
    return &schedulor.first_task;
}