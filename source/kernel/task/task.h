#ifndef __TASK_H
#define __TASK_H
#include "types.h"
#include "list.h"
#define TASK_TIME_SLICE_DEFAULT		10			// 时间片计数
#define TASK_SCHED_POLICY_DEFAULT   SCHED_FIFO
#define PID_MAX_NR 1024
#define TASK_MAX_NR PID_MAX_NR
#define BITMAP_SIZE ((PID_MAX_NR + 7) / 8)
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

typedef struct _pidalloc_t
{
    uint8_t bitmap[BITMAP_SIZE]; // 位图，记录 PID 的使用情况
} pidalloc_t;

typedef struct _task_attr_t{
    enum {
        SCHED_FIFO,
    }sched_policy;
    int time_slice;			// 时间片
    uint32_t stack_size;
}task_attr_t;

typedef struct _task_t
{
    enum 
    {
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

    int sleep_ticks;		// 睡眠时间
    
	int slice_ticks;		// 递减时间片计数




} task_t;

typedef struct _schedulor_t
{
    task_t* cur_task;

    list_t ready_list;
    list_t sleep_list;

    task_t idle_task;
    task_t first_task;

}schedulor_t;

/*pid*/
extern pidalloc_t pidallocter;
void pidalloc_init(pidalloc_t *alloc);
int pid_is_allocated(pidalloc_t *alloc, int pid);
int pid_alloc(pidalloc_t *alloc);
void pid_free(pidalloc_t *alloc, int pid);
void pidalloc_print(pidalloc_t *alloc);


int task_init(task_t *task,int type,ph_addr_t entry,task_attr_t *attr);
task_t* task_alloc(void);
void task_free(task_t* task);


void sched_init(void);
void switch_to_tss (uint32_t tss_selector);
/**
 * 切换任务
 **/
void task_switch_from_to (task_t * from, task_t * to);
task_t* get_cur_task(void);
void set_cur_task(task_t* task);
task_t* get_ready_task(void);
void set_task_to_ready_list(task_t* task);
void schedul(void);
void task_time_tick(void);
#endif