#ifndef __PID_H
#define __PID_H
#include "types.h"
/*pid*/
#define PID_MAX_NR 1024
#define TASK_MAX_NR PID_MAX_NR
//pid位图
#define BITMAP_SIZE ((PID_MAX_NR + 7) / 8) 
typedef struct _pidalloc_t
{
    uint8_t bitmap[BITMAP_SIZE]; // 位图，记录 PID 的使用情况
} pidalloc_t;
void pidalloc_init(pidalloc_t *alloc);

/**
 * 检查指定 PID 是否已被分配
 * @param alloc PID 分配器
 * @param pid 待检查的 PID
 * @return true - 已分配，false - 未分配
 */
int pid_is_allocated(pidalloc_t *alloc, int pid) ;

/**
 * 分配一个 PID
 * @param alloc PID 分配器
 * @return 分配的 PID，-1 表示没有可用的 PID
 */
int pid_alloc(pidalloc_t *alloc);

/**
 * 释放指定的 PID
 * @param alloc PID 分配器
 * @param pid 待释放的 PID
 */
void pid_free(pidalloc_t *alloc, int pid);

/**
 * 打印所有被占用的 PID
 * @param alloc PID 分配器
 */
void pidalloc_print(pidalloc_t *alloc);

#endif