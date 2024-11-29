#include "task.h"
#include "err.h"
#include "cpu.h"
#include "string.h"
#include "mem/memory.h"
#include "cpu_cfg.h"

task_t task_pool[TASK_MAX_NR];


int tss_init(task_t* task,int type,ph_addr_t entry,task_attr_t *attr)
{
    int tss_sel = gdt_alloc_desc();
    if(tss_sel<0)
    {
        dbg_error("tss desc alloc failed:%s\r\n",err_str[DESC_TSS_CROSS_BORDER]);
        return DESC_TSS_CROSS_BORDER;
    }
    gdt_set_tss(tss_sel,(ph_addr_t)&task->tss,sizeof(tss_t));
    memset(&task->tss,0,sizeof(tss_t));

    ph_addr_t kernel_stack_base = mm_alloc_one_page();
    if(kernel_stack_base < 0)
    {
        dbg_error("task alloc kernel stack fail:%s\r\n",err_str[MEM_NOT_ENOUGH]);
        gdt_free_desc(tss_sel);
        return MEM_NOT_ENOUGH;
    }

    int code_sel,data_sel;
    if(type == KERNEL)
    {
        code_sel = SELECTOR_KERNEL_CODE_SEG;
        data_sel = SELECTOR_KERNEL_DATA_SEG;
        task->tss.esp = task->tss.esp0 = kernel_stack_base + MEM_PAGE_SIZE -1;
    }else{
        code_sel = SELECTOR_USR_CODE_SEG;
        data_sel = SELECTOR_USR_DATA_SEG;
        task->tss.esp = 0xFFFFFFFF;
        task->tss.esp0 = kernel_stack_base + MEM_PAGE_SIZE -1;
    }

    ph_addr_t page_dir = mmu_create_task_pgd();
    if(page_dir < 0)
    {
        dbg_error("task page dir alloc failed:%s\r\n",err_str[MEM_NOT_ENOUGH]);
        gdt_free_desc(tss_sel);
        mm_free_one_page(kernel_stack_base);
        return MEM_NOT_ENOUGH;
    }

    task->tss.eip = entry;
    task->tss.eflags = EFLAGS_DEFAULT| EFLAGS_IF;
    task->tss.ss0 = SELECTOR_KERNEL_CODE_SEG;
    task->tss.es = task->tss.ss = task->tss.ds = task->tss.fs 
            = task->tss.gs = data_sel;   // 全部采用同一数据段
    task->tss.cs = code_sel; 

    task->tss.cr3 = page_dir;
    task->tss_sel = tss_sel;
    task->entry = entry;

    return ERROR_OK;   
}

int task_init(task_t *task,int type,ph_addr_t entry,task_attr_t *attr)
{
    if(!task){
        return TASK_NULL;
    }
    if(attr)
    {
        task->attr.sched_policy = attr->sched_policy;
        task->attr.stack_size = attr->stack_size;
        task->slice_ticks =task->attr.time_slice = attr->time_slice;
    }
    else{
        task->attr.sched_policy = TASK_SCHED_POLICY_DEFAULT;
        task->attr.stack_size = MEM_PAGE_SIZE;
        task->slice_ticks = task->attr.time_slice = TASK_TIME_SLICE_DEFAULT;
    }
    int ret;
    ret = tss_init(task,type,entry,attr);
    if(ret != ERROR_OK)
    {
        dbg_error("%s\r\n",err_str[ret]);
        return ret;
    }
    task->pid = pid_alloc(&pidallocter);
    task->type = type;
    task->state = TASK_CREATED;
    return 0;
}

task_t* task_alloc(void)
{
    for (int i = 0; i < TASK_MAX_NR; i++)
    {
        if(task_pool[i].type == TASKNONE)
        {
            return &task_pool[i];
        }
    }
    dbg_error("there is none in task pool\r\n");
    return NULL;
}

void task_free(task_t* task)
{
    task->type = TASKNONE;
}

void create_kernel_process(task_t* task,process_func_t func)
{
    irq_state_t state = irq_enter_protection();
    task_init(task, KERNEL, func, NULL);
    set_task_to_ready_list(task);
    irq_leave_protection(state);
}

