#include "task.h"
#include "err.h"
#include "cpu.h"
#include "string.h"
#include "mem/memory.h"
#include "cpu_cfg.h"
#include "math.h"

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

    //给任务分配内核栈空间
    ph_addr_t kernel_stack_base = mm_alloc_one_page();
    if(kernel_stack_base < 0)
    {
        dbg_error("task alloc kernel stack fail:%s\r\n",err_str[MEM_NOT_ENOUGH]);
        gdt_free_desc(tss_sel);
        return MEM_NOT_ENOUGH;
    }

    uint32_t page_dir = mmu_create_task_pgd();
    if(page_dir ==NULL)
    {
        dbg_error("task alloc page dir fail\r\n");
        mm_free_one_page(kernel_stack_base);
        gdt_free_desc(tss_sel);
        return -1;
    }

    task->tss.cr3 = page_dir;
    task->tss_sel = tss_sel;

    // 根据不同的权限选择不同的访问选择子
    int code_sel, data_sel;
    if (type == KERNEL) {
        code_sel = SELECTOR_KERNEL_CODE_SEG;
        data_sel = SELECTOR_KERNEL_DATA_SEG;
        task->tss.esp = kernel_stack_base+MEM_PAGE_SIZE;
    } else {
        // 注意加了RP3,不然将产生段保护错误
        code_sel = SELECTOR_USR_CODE_SEG;
        data_sel = SELECTOR_USR_DATA_SEG;
        int stack_page_count = task->attr.stack_size/MEM_PAGE_SIZE;
        mmblock(task,USR_STACK_TOP-task->attr.stack_size,stack_page_count);
        task->tss.esp = USR_STACK_TOP;
    }

    task->tss.eip = entry;
    task->tss.esp0 = kernel_stack_base + MEM_PAGE_SIZE;
    task->tss.ss0 = SELECTOR_KERNEL_DATA_SEG;
    task->tss.eip = entry;
    task->tss.eflags = EFLAGS_DEFAULT| EFLAGS_IF;
    task->tss.es = task->tss.ss = task->tss.ds = task->tss.fs 
            = task->tss.gs = data_sel;   // 全部采用同一数据段
    task->tss.cs = code_sel; 
    task->tss.iomap = 0;

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
        task->attr.stack_size = align_up_to(attr->stack_size,MEM_PAGE_SIZE) ;
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
    task->entry = entry;
    
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
    ph_addr_t esp = mm_alloc_one_page();
    task_init(task, KERNEL, func ,NULL);
    set_task_to_ready_list(task);
    irq_leave_protection(state);
}

ph_addr_t task_get_page_dir(task_t *task)
{
    return task->tss.cr3;
}

int sys_getpid(void)
{
    return get_cur_task()->pid;
}