#ifndef __SEMAPHOR_H
#define __SEMAPHOR_H
#include "cpu.h"
#include "task/task.h"
#include "list.h"
#include "_time.h"
typedef struct _sem_t {
    int count;				// 信号量计数
    list_t wait_list;		// 等待的进程列表

    tm_t *tmo;            //超时时间
    list_node_t node;
}sem_t;
extern list_t sem_timewait_list;
int sys_sem_count (sem_t * sem);
void sys_sem_wait(sem_t *sem);
int sys_sem_timedwait(sem_t *sem, tm_t *tmo);
int sys_sem_trywait(sem_t *sem);
void sys_sem_notify(sem_t *sem);
void sys_sem_init (sem_t * sem, int init_count);
void task_ipc_init(void);
void sem_time_check(void);

#endif

