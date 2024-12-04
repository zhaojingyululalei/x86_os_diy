#include "task.h"
#include "err.h"
#include "cpu.h"
#include "string.h"
#include "mem/memory.h"
#include "cpu_cfg.h"
#include "math.h"
#include "cpu_instr.h"

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
        if (stack_top == NULL) // 没指定用户栈，自己开辟
        {
            int stack_page_count = task->attr.stack_size / MEM_PAGE_SIZE;
            // 给用户分配用户栈空间
            mmblock(task, USR_STACK_TOP - task->attr.stack_size, stack_page_count);
            task->tss.esp = USR_STACK_TOP;
        }
        else
        {
            task->tss.esp = stack_top; // 指定了用户栈，用指定的
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
        task->attr.stack_size = 2*MEM_PAGE_SIZE;
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
    task_init(task, KERNEL, func, NULL, NULL);
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
    // 父进程，由于retf的缘故，会把那5个参数pop出去
    // 子进程不会，因此手动+5
    int ret = task_init(child, USR, frame->eip, frame->esp + sizeof(uint32_t) * CALL_GATE_PRAM_COUNT, &parent->attr);
    if (ret < 0)
    {
        task_free(child);
        irq_leave_protection(state);
        return -1;
    }
    tss_t *tss = &child->tss;
    tss->eax = 0; // 子进程返回0
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

    // 拷贝父进程页表，并把用户空间页表全部改写为只读,用于触发页异常。在异常中完成写时复制操作
    ph_addr_t stack_start = USR_STACK_TOP - parent->attr.stack_size; // 栈开始地址

    // 获取父进程和子进程的页目录
    pde_t *parent_pgd = task_get_page_dir(parent);
    pde_t *child_pgd = task_get_page_dir(child);
    int stack_page_count = parent->attr.stack_size / MEM_PAGE_SIZE;
    // 栈的页表拷贝（特殊处理，不遵循写时复制）
    mmu_cpy_page_dir(parent_pgd, child_pgd, stack_start, stack_page_count);

    // 获取用户空间页目录的起始和结束指针
    pde_t *parent_start_pgd = mmu_from_vm_get_pde(parent_pgd, USR_ENTRY_BASE);
    pde_t *parent_pgd_end = mmu_from_vm_get_pde(parent_pgd, stack_start);
    pde_t *child_start_pgd = mmu_from_vm_get_pde(child_pgd, USR_ENTRY_BASE);

    // 遍历用户空间页目录
    for (pde_t *p = parent_start_pgd, *c = child_start_pgd; p < parent_pgd_end; p++, c++)
    {
        if (!p->present)
        {
            continue; // 父进程页目录项不存在，跳过
        }

        // 子页目录项拷贝父页目录项
        c->v = p->v;

        // 分配新的页表
        ph_addr_t page_table = mm_alloc_one_page();
        c->phy_pt_addr = page_table >> 12;

        // 获取父页表和子页表的起始地址
        pte_t *parent_start_pte = (pte_t *)((p->phy_pt_addr) << 12);
        pte_t *child_start_pte = (pte_t *)((c->phy_pt_addr) << 12);

        // 遍历页表项
        for (int i = 0; i < 1024; i++)
        {
            if (!parent_start_pte[i].present)
            {
                child_start_pte[i].v = 0; // 父页表项不存在，子页表项置 0
                continue;
            }

            // 拷贝父页表项到子页表项
            child_start_pte[i] = parent_start_pte[i];

            // 设置子页表项为只读并标记为写时复制
            child_start_pte[i].write_disable = 0;                 // 设置为只读
            child_start_pte[i].ignore = PTE_IGNORE_COPY_ON_WRITE; // 标记为写时复制
        }
    }

    set_task_to_ready_list(child);
    irq_leave_protection(state);
    return child->pid;
}

#include "fs/fs.h"
#include "elf.h"
/**
 * 给应用分配内存
 * 加载应用
 * 建立虚拟地址映射关系
 * 返回应用入口地址
 */

static int load_phdr(int fd,Elf32_Phdr* phdr,task_t* task)
{
    int ret;
    ph_addr_t start_vm = phdr->p_vaddr;
    uint32_t memsize = phdr->p_memsz;
    memsize = align_up_to(memsize,MEM_PAGE_SIZE);
    int block = memsize / MEM_PAGE_SIZE;
    ret = mmblock(task,start_vm,block);
    if(ret < 0)
    {
        dbg_error("mmblock err\r\n");
        return -1;
    }

    // 调整当前的读写位置
    if (sys_lseek(fd, phdr->p_offset, 0) < 0) {
        dbg_error("read file failed");
        return -1;
    }

    //拷贝程序
    uint32_t vaddr = phdr->p_vaddr;
    uint32_t size = phdr->p_filesz;
    while (size > 0) {
        int curr_size = (size > MEM_PAGE_SIZE) ? MEM_PAGE_SIZE : size;

        uint32_t paddr = mmu_get_phaddr(task->tss.cr3,vaddr);

        // 注意，这里用的页表仍然是当前的
        if (sys_read(fd, (char *)paddr, curr_size) != curr_size) {
            dbg_error("read file failed");
            return -1;
        }

        size -= curr_size;
        vaddr += curr_size;
    }
    //存在bss区
    if(phdr->p_memsz>phdr->p_filesz)
    {
        //清零
        memset(vaddr,0,phdr->p_memsz-phdr->p_filesz);
    }
    return 0;


}
static ph_addr_t load_app_elf(task_t *task, const char *path)
{
    ph_addr_t entry = 0;
    Elf32_Ehdr elf_hdr;
    Elf32_Phdr elf_phdr;

    int fd = sys_open(path, 0);
    if (fd < 0)
    {
        dbg_error("open app xxx.elf fail\r\n");
        return 0;
    }
    // 读elf头
    int cnt = sys_read(fd, (char *)&elf_hdr, sizeof(Elf32_Ehdr));
    if (cnt != sizeof(Elf32_Ehdr))
    {
        dbg_error("read app elf head wrong\r\n");
        return 0;
    }

    // 检查elf文件格式
    // 检查magic
    if ((elf_hdr.e_ident[0] != ELF_MAGIC) || (elf_hdr.e_ident[1] != 'E') || (elf_hdr.e_ident[2] != 'L') || (elf_hdr.e_ident[3] != 'F'))
    {
        dbg_error("check elf indent failed.\r\n");
        return 0;
    }
    // 检查app架构
    if (elf_hdr.e_machine != EM_386)
    {
        dbg_error("app incompatible machine architecture\r\n");
        return 0;
    }
    // 检查文件类型
    if (elf_hdr.e_type != ET_EXEC)
    {
        dbg_error("it is not an exec file\r\n");
        return 0;
    }
    // 检查入口地址
    if (elf_hdr.e_entry < USR_ENTRY_BASE)
    {
        dbg_error("app entry wrong\r\n");
        return 0;
    }
    else
    {
        entry = elf_hdr.e_entry;
    }

    // 读取程序头并解析
    uint32_t e_phoff = elf_hdr.e_phoff;
    for (int i = 0; i < elf_hdr.e_phnum; i++, e_phoff += elf_hdr.e_phentsize)
    {
        if (sys_lseek(fd, e_phoff, 0) < 0)
        {
            dbg_error("read file failed");
            return 0;
        }

        // 读取程序头后解析，这里不用读取到新进程的页表中，因为只是临时使用下
        cnt = sys_read(fd, (char *)&elf_phdr, sizeof(Elf32_Phdr));
        if (cnt != sizeof(Elf32_Phdr))
        {
            dbg_error("read file failed");
            return 0;
        }

        // 简单做一些检查，如有必要，可自行加更多
        // 主要判断是否是可加载的类型，并且要求加载的地址必须是用户空间
        if ((elf_phdr.p_type != PT_LOAD) || (elf_phdr.p_vaddr < USR_ENTRY_BASE))
        {
            continue;
        }

        // 加载当前程序头
        int err = load_phdr(fd, &elf_phdr, task);
        if (err < 0)
        {
            dbg_error("load program hdr failed");
            return 0;
        }

    }

    sys_close(fd);

    return entry;
}

static ph_addr_t cpy_argv(ph_addr_t stack_base_ph, char *const *argv,int* argc)
{
    if(argv == NULL)
    {
        return 0;
    }
    ph_addr_t *cur_pos = (ph_addr_t *)stack_base_ph; // 指向新栈的起始位置，用于存储指针
    char *content_pos = (char *)(cur_pos + 128);   // 假设 argv 的内容从指针区域后开始（预留空间）
    char **cur_ptr = argv;

    //复制参数指针（指向新栈中的内容区域）
    while (*cur_ptr) {
        uint32_t len = strlen(*cur_ptr) + 1; //算上 '/0'
        if(len >= 76)
        {
            dbg_error("argv len is so long\r\n");
            return 0;
        }
        *cur_pos = (ph_addr_t)content_pos; 
        strncpy(content_pos, *cur_ptr, len); 
        content_pos += len; 
        cur_pos++;          
        cur_ptr++;
        *argc+=1;
        if(*argc ==20)
        {
            dbg_error("too much argv count");
            return 0;
        }
    }

    // 添加 NULL 终止符
    *cur_pos++ = 0;

    // 返回最终的新栈基地址
    return (ph_addr_t)cur_pos;
}


int sys_execve(const char *path, char *const *argv, char *const *env)
{
    irq_state_t state = irq_enter_protection();
    ph_addr_t entry;
    // 创建新页表，释放旧页表
    task_t *cur_task = get_cur_task();
    ph_addr_t old_page_dir = cur_task->tss.cr3;
    ph_addr_t new_page_dir = mmu_create_task_pgd();
    if (new_page_dir == NULL)
    {
        dbg_warning("execve create new page dir fail\r\n");
        irq_leave_protection(state);
        return -1;
    }

    //在切换页表前，处理参数，否则页表切换后，参数没了
    task_attr_t* cur_attr = &cur_task->attr;
    ph_addr_t stack_base_vm = USR_STACK_TOP - cur_attr->stack_size;
    int stack_page_count = cur_attr->stack_size / MEM_PAGE_SIZE;
    ph_addr_t stack_base_ph = mm_alloc_pages(stack_page_count);
    if(stack_base_ph == NULL)
    {
        dbg_error("execve alloc stack space fail\r\n");
        irq_leave_protection(state);
        return -1;
    }
    int argc = 0,envc = 0;
    char** new_argv = stack_base_ph;
    ph_addr_t cur_pos = cpy_argv(stack_base_ph,argv,&argc);
    if(cur_pos == NULL)
    {
        dbg_error("execve cpyarg fail\r\n");
        irq_leave_protection(state);
        return -1;
    }
    char** new_env = stack_base_ph+2048;
    ph_addr_t test_tmp = cpy_argv(stack_base_ph+2048,env,&envc);
    if(test_tmp == NULL)
    {
        dbg_error("execve cpyarg fail\r\n");
        irq_leave_protection(state);
        return -1;
    }
    ph_addr_t stack_top_ph = stack_base_ph + cur_attr->stack_size;

    char new_path[128] ;
    strncpy(new_path,path,strlen(path));

    //argc 和 argv复制到栈顶
    *((uint32_t*)(stack_top_ph-sizeof(ph_addr_t))) = stack_base_vm;
    *((uint32_t*)(stack_top_ph-sizeof(ph_addr_t)*2)) = argc;

    ph_addr_t stack_top = USR_STACK_TOP - sizeof(ph_addr_t)*2;

    //处理完参数后，切换新页表，销毁旧页表
    cur_task->tss.cr3 = new_page_dir;
    write_cr3(new_page_dir);
    irq_leave_protection(state);
    mmu_destory_task_pgd((pde_t *)old_page_dir);

    //加载程序
    entry = load_app_elf(cur_task,new_path);
    if(entry == NULL)
    {
        dbg_error("load app elf fail\r\n");
        return -1;
    }

    //给栈空间建立虚拟地址映射关系（因为是新页表了）
    for (int i = 0; i < stack_page_count; i++)
    {
        mmu_memory_map(cur_task->tss.cr3,stack_base_vm,stack_base_ph,1,1);
        stack_base_ph +=MEM_PAGE_SIZE;
        stack_base_vm += MEM_PAGE_SIZE;
    }

    syscall_frame_t * frame = (syscall_frame_t *)(cur_task->tss.esp0 - sizeof(syscall_frame_t));
    frame->eip = entry;
    frame->eax = frame->ebx = frame->ecx = frame->edx = 0;
    frame->esi = frame->edi = frame->ebp = 0;
    frame->eflags = EFLAGS_DEFAULT| EFLAGS_IF;  // 段寄存器无需修改

    // 内核栈不用设置，保持不变，后面调用memory_destroy_uvm并不会销毁内核栈的映射。
    // 但用户栈需要更改, 同样要加上调用门的参数压栈空间
    frame->esp = stack_top - sizeof(uint32_t)*CALL_GATE_PRAM_COUNT;

    return 0;
}