

#include "task.h"
#include "debug.h"
#include "cpu.h"
#include "cpu_instr.h"
static schedulor_t schedulor;

static void idle_func(void)
{
    while (1)
    {
        hlt();
    }
}
static idle_task_init(void)
{
    task_init(&schedulor.idle_task, KERNEL, idle_func, NULL);
}
static void first_task_init(void)
{
    
    task_init(&schedulor.first_task,KERNEL,0,NULL);
    set_cur_task(&schedulor.first_task);
    write_tr(schedulor.first_task.tss_sel);
}
void sched_init(void)
{
    pidalloc_init(&pidallocter);

    list_init(&schedulor.ready_list);
    list_init(&schedulor.sleep_list);
    schedulor.cur_task = NULL;
    idle_task_init();
    first_task_init();
}

task_t *get_cur_task(void)
{
    return schedulor.cur_task;
}
void set_cur_task(task_t *task)
{
    schedulor.cur_task = task;
}

task_t *get_ready_task(void)
{
    list_node_t *fnode = list_remove_first(&schedulor.ready_list);
    if (fnode == NULL)
    {
        return NULL;
    }
    task_t *ftask = list_node_parent(fnode, task_t, node);
    return ftask;
}
void set_task_to_ready_list(task_t *task)
{
    if (!task)
    {
        dbg_error("task is empty\r\n");
        return;
    }
    list_insert_last(&schedulor.ready_list, &task->node);
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
    switch_to_tss(to->tss_sel);
}
void schedul(void)
{

    task_t *pre_task = get_cur_task();
    task_t *cur_task = get_ready_task();
    if (pre_task == cur_task)
    {
        return;
    }
    if(cur_task == NULL)
    {
        cur_task = &schedulor.idle_task;
    }
    set_cur_task(cur_task);
    if(pre_task != &schedulor.idle_task)
    {

        set_task_to_ready_list(pre_task);
    }
    task_switch_from_to(pre_task, cur_task);
}

void task_time_tick(void)
{
    irq_state_t state = irq_enter_protection();

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