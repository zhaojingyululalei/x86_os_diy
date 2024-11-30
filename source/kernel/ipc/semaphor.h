#ifndef __SEMAPHOR_H
#define __SEMAPHOR_H
#include "cpu.h"
#include "task/task.h"
#include "list.h"
typedef struct _sem_t {
    int count;				// 信号量计数
    list_t wait_list;		// 等待的进程列表
}sem_t;

int sys_sem_count (sem_t * sem);
void sys_sem_wait(sem_t *sem);
void sys_sem_notify(sem_t *sem);
void sys_sem_init (sem_t * sem, int init_count);

#endif

