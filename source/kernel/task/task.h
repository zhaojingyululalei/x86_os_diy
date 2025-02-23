#ifndef __TASK_H
#define __TASK_H
#include "types.h"
#include "list.h"
#include "cpu_cfg.h"
#define TASK_TIME_SLICE_DEFAULT		10			// 时间片计数
#define TASK_SCHED_POLICY_DEFAULT   SCHED_FIFO
#define USR_STACK_SIZE_DEFAULT  (2 * MEM_PAGE_SIZE)
#define KERNEL_STACK_SIZE_DEFAULT   MEM_PAGE_SIZE
#define TASK_OFILE_NR				128			// 最多支持打开的文件数量

#include "pid.h"
#include "fs/file.h"
#define DEFINE_PROCESS_FUNC(name)            void* name(void* arg)
typedef void*(*process_func_t)(void*);
/**
 * tss描述符
 */
typedef struct _tss_t
{
    uint32_t pre_link;
    uint32_t esp0, ss0, esp1, ss1, esp2, ss2;
    uint32_t cr3;
    uint32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint32_t iomap;
} tss_t;



typedef struct _task_attr_t{
    enum {
        SCHED_FIFO,
    }sched_policy;
    int time_slice;			// 时间片
    uint32_t stack_size;
}task_attr_t;
#define TASK_NOT_COLLECT 0
#define TASK_COLLECTING 1
#include "mem/malloc.h"
typedef struct _task_t
{
    enum 
    {
        TASK_NONE,
        TASK_CREATED,
        TASK_RUNNING,
        TASK_SLEEP,
        TASK_READY,
        TASK_WAITING,
        TASK_ZOMBIE,
    } state;

    enum {
        TASKNONE,
        KERNEL,
        USR
    }type;

    task_attr_t attr;

    ph_addr_t entry;
    int pid;
    tss_t tss;
    uint16_t tss_sel;
    list_node_t node;
    list_node_t all_node; //创建就加入all list，直到wait，才离开

    int sleep_ticks;		// 睡眠时间
    
	int slice_ticks;		// 递减时间片计数

    struct _task_t* parent;
    int status;     //exit 进程退出状态值
    int wait_flag; //父进程正在回收子进程的标志

    ph_addr_t heap_s;
    ph_addr_t heap_e;

    flmallocor_t flmlk; //malloc分配器
    file_t * file_table[TASK_OFILE_NR];	// 任务最多打开的文件数量

} task_t;

typedef struct _schedulor_t
{
    task_t* cur_task;

    list_t ready_list;
    list_t sleep_list;
    list_t all_list;

    task_t idle_task;
    task_t first_task;

}schedulor_t;



extern  pidalloc_t pidallocter;
/*task*/
int task_init(task_t *task, int type, ph_addr_t entry, ph_addr_t stack_top, task_attr_t *attr);
task_t* task_alloc(void);
void task_free(task_t* task);
void create_kernel_process(task_t* task,process_func_t func);
ph_addr_t task_get_page_dir(task_t* task);
void first_task_init(void);
static inline task_t* task_get_parent(task_t* task)
{
    return task->parent;
}
task_t* task_all_list_get_first();
task_t* task_all_list_get_next(task_t* cur);
task_t* get_init_task();
void task_collect(task_t* task);
/**
 * 任务调度
 **/
void sched_init(void);
void switch_to_tss (uint32_t tss_selector);
void task_switch_from_to (task_t * from, task_t * to);
task_t* get_cur_task(void);
void set_cur_task(task_t* task);
task_t* get_ready_task(void);
void set_task_to_ready_list(task_t* task);
void set_task_to_all_list(task_t* task);
void remove_task_from_ready_list(task_t* task);
void remove_task_from_all_list(task_t* task);
void schedul(void);
void task_time_tick(void);
void task_goto_sleep(task_t* task);
void task_wakeup(task_t* task);

file_t * task_file (int fd);
int task_alloc_fd (file_t * file);
void task_remove_fd (int fd);

/*系统调用*/
int sys_sleep_ms(int time);
int sys_getpid(void);
int sys_fork(void);
int sys_execve(const char *path, char *const *argv, char *const *env) ;
int sys_yield(void);
void sys_exit(int status);
int sys_wait(int* status) ;
#endif
