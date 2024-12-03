#include "task.h"
#include "err.h"
#include "cpu.h"
#include "string.h"
#include "mem/memory.h"
#include "cpu_cfg.h"
#include "math.h"

task_t task_pool[TASK_MAX_NR];

int tss_init(task_t *task, int type, ph_addr_t entry, ph_addr_t stack_top, task_attr_t *attr)
{
    int tss_sel = gdt_alloc_desc();
    if (tss_sel < 0)
    {
        dbg_error("tss desc alloc failed:%s\r\n", err_str[DESC_TSS_CROSS_BORDER]);
        return -1;
    }
    gdt_set_tss(tss_sel, (ph_addr_t)&task->tss, sizeof(tss_t));
    memset(&task->tss, 0, sizeof(tss_t));

    // 给任务分配内核栈空间
    ph_addr_t kernel_stack_base = mm_alloc_one_page();
    if (kernel_stack_base < 0)
    {
        dbg_error("task alloc kernel stack fail:%s\r\n", err_str[MEM_NOT_ENOUGH]);
        gdt_free_desc(tss_sel);
        return -1;
    }

    uint32_t page_dir = mmu_create_task_pgd();
    if (page_dir == NULL)
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
    if (type == KERNEL)
    {
        code_sel = SELECTOR_KERNEL_CODE_SEG;
        data_sel = SELECTOR_KERNEL_DATA_SEG;
        task->tss.esp = kernel_stack_base + MEM_PAGE_SIZE;
    }
    else
    {
        // 注意加了RP3,不然将产生段保护错误
        code_sel = SELECTOR_USR_CODE_SEG;
        data_sel = SELECTOR_USR_DATA_SEG;
        if (stack_top == NULL)//没指定用户栈，自己开辟
        {
            int stack_page_count = task->attr.stack_size / MEM_PAGE_SIZE;
            // 给用户分配用户栈空间
            mmblock(task, USR_STACK_TOP - task->attr.stack_size, stack_page_count);
            task->tss.esp = USR_STACK_TOP;
        }
        else{
            task->tss.esp = stack_top;//指定了用户栈，用指定的
        }
    }

    task->tss.eip = entry;
    task->tss.esp0 = kernel_stack_base + MEM_PAGE_SIZE;
    task->tss.ss0 = SELECTOR_KERNEL_DATA_SEG;
    task->tss.eip = entry;
    task->tss.eflags = EFLAGS_DEFAULT | EFLAGS_IF;
    task->tss.es = task->tss.ss = task->tss.ds = task->tss.fs = task->tss.gs = data_sel; // 全部采用同一数据段
    task->tss.cs = code_sel;
    task->tss.iomap = 0;

    return 0;
}

int task_init(task_t *task, int type, ph_addr_t entry, ph_addr_t stack_top, task_attr_t *attr)
{
    if (!task)
    {
        return -1;
    }
    if (attr)
    {
        task->attr.sched_policy = attr->sched_policy;
        task->attr.stack_size = align_up_to(attr->stack_size, MEM_PAGE_SIZE);
        task->slice_ticks = task->attr.time_slice = attr->time_slice;
    }
    else
    {
        task->attr.sched_policy = TASK_SCHED_POLICY_DEFAULT;
        task->attr.stack_size = MEM_PAGE_SIZE;
        task->slice_ticks = task->attr.time_slice = TASK_TIME_SLICE_DEFAULT;
    }
    int ret;
    ret = tss_init(task, type, entry, stack_top, attr);
    if (ret != ERROR_OK)
    {
        dbg_error("%s\r\n", err_str[ret]);
        return ret;
    }
    task->pid = pid_alloc(&pidallocter);
    task->type = type;
    task->state = TASK_CREATED;
    task->entry = entry;

    return 0;
}

task_t *task_alloc(void)
{
    for (int i = 0; i < TASK_MAX_NR; i++)
    {
        if (task_pool[i].type == TASKNONE)
        {
            return &task_pool[i];
        }
    }
    dbg_error("there is none in task pool\r\n");
    return NULL;
}

void task_free(task_t *task)
{
    task->type = TASKNONE;
}

void create_kernel_process(task_t *task, process_func_t func)
{
    irq_state_t state = irq_enter_protection();
    task_init(task, KERNEL, func,NULL, NULL);
    set_task_to_ready_list(task);
    irq_leave_protection(state);
}

ph_addr_t task_get_page_dir(task_t *task)
{
    return task->tss.cr3;
}

int sys_getpid(void)
{
    irq_state_t state = irq_enter_protection();
    int pid = get_cur_task()->pid;
    irq_leave_protection(state);
    return pid;

}
#include "syscall.h"
#include "cpu_cfg.h"

int sys_fork(void)
{
    irq_state_t state = irq_enter_protection();
    task_t *parent = get_cur_task();
    task_t *child = task_alloc();
    if (child == NULL)
    {
        irq_leave_protection(state);
        return -1;
    }
    syscall_frame_t *frame = (syscall_frame_t *)(parent->tss.esp0 - sizeof(syscall_frame_t));
    //父进程，由于retf的缘故，会把那5个参数pop出去
    //子进程不会，因此手动+5
    int ret = task_init(child, USR, frame->eip, frame->esp + sizeof(uint32_t)*CALL_GATE_PRAM_COUNT,&parent->attr);
    if(ret <0)
    {
        task_free(child);
        irq_leave_protection(state);
        return -1;
    }
    tss_t* tss = &child->tss;
    tss->eax = 0;                       // 子进程返回0
    tss->ebx = frame->ebx;
    tss->ecx = frame->ecx;
    tss->edx = frame->edx;
    tss->esi = frame->esi;
    tss->edi = frame->edi;
    tss->ebp = frame->ebp;

    tss->cs = frame->cs;
    tss->ds = frame->ds;
    tss->es = frame->es;
    tss->fs = frame->fs;
    tss->gs = frame->gs;
    tss->eflags = frame->eflags;

    child->parent = parent;

    //拷贝父进程页表，并把用户空间页表全部改写为只读,用于触发页异常。在异常中完成写时复制操作
    ph_addr_t stack_start = USR_STACK_TOP - parent->attr.stack_size; // 栈开始地址

    // 获取父进程和子进程的页目录
    pde_t* parent_pgd = task_get_page_dir(parent);
    pde_t* child_pgd = task_get_page_dir(child);
    int stack_page_count = parent->attr.stack_size/MEM_PAGE_SIZE;
    // 栈的页表拷贝（特殊处理，不遵循写时复制）
    mmu_cpy_page_dir(parent_pgd, child_pgd, stack_start, stack_page_count);

    // 获取用户空间页目录的起始和结束指针
    pde_t* parent_start_pgd = mmu_from_vm_get_pde(parent_pgd, USR_ENTRY_BASE);
    pde_t* parent_pgd_end = mmu_from_vm_get_pde(parent_pgd, stack_start);
    pde_t* child_start_pgd = mmu_from_vm_get_pde(child_pgd, USR_ENTRY_BASE);

    // 遍历用户空间页目录
    for (pde_t* p = parent_start_pgd, *c = child_start_pgd; p < parent_pgd_end; p++, c++) {
        if (!p->present) {
            continue; // 父进程页目录项不存在，跳过
        }

        // 子页目录项拷贝父页目录项
        c->v = p->v;

        // 分配新的页表
        ph_addr_t page_table = mm_alloc_one_page();
        c->phy_pt_addr = page_table >> 12;

        // 获取父页表和子页表的起始地址
        pte_t* parent_start_pte = (pte_t*)((p->phy_pt_addr) << 12);
        pte_t* child_start_pte = (pte_t*)((c->phy_pt_addr) << 12);

        // 遍历页表项
        for (int i = 0; i < 1024; i++) {
            if (!parent_start_pte[i].present) {
                child_start_pte[i].v = 0; // 父页表项不存在，子页表项置 0
                continue;
            }

            // 拷贝父页表项到子页表项
            child_start_pte[i] = parent_start_pte[i];

            // 设置子页表项为只读并标记为写时复制
            child_start_pte[i].write_disable = 0;  // 设置为只读
            child_start_pte[i].ignore = PTE_IGNORE_COPY_ON_WRITE; // 标记为写时复制
        }
    }

    set_task_to_ready_list(child);
    irq_leave_protection(state);
    return child->pid;
}