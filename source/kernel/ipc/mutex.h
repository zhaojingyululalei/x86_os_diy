#ifndef __MUTEX_H
#define __MUTEX_H

#include "task/task.h"
#include "list.h"
typedef struct _mutex_t {
    task_t * owner;
    int locked_count;
    list_t wait_list;
}mutex_t;

void sys_mutex_init (mutex_t * mutex);
void sys_mutex_lock (mutex_t * mutex);
void sys_mutex_unlock (mutex_t * mutex);
#endif


