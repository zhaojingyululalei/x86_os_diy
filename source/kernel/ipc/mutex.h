#ifndef __MUTEX_H
#define __MUTEX_H

#include "task/task.h"
#include "list.h"
typedef struct _mutex_t {
    struct _task_t * owner;
    int locked_count;
    list_t wait_list;
}mutex_t;
/*可重入的锁*/
void sys_mutex_init (mutex_t * mutex);
void sys_mutex_lock (mutex_t * mutex);
void sys_mutex_unlock (mutex_t * mutex);
void sys_mutex_destory (mutex_t * mutex);
#endif


