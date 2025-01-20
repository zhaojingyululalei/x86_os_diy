#ifndef ___STDLIB_H
#define ___STDLIB_H
#include "types.h"
#include "../kernel/ipc/mutex.h"
#include "../kernel/ipc/semaphor.h"
typedef struct _syscall_args_t
{
    int id;
    int arg0;
    int arg1;
    int arg2;
    int arg3;
} syscall_args_t;

int sleep(int ms);
int getpid(void);
void printf_tmp(char *fmt, int arg);
int fork(void);
int execve(const char *path, char *const *argv, char *const *env);
int yield(void);
void exit(int status);
int wait(int *status);
void *malloc(int size);
void free(void *ptr);

void mutex_init(mutex_t *mutex);

void mutex_lock(mutex_t *mutex);

void mutex_unlock(mutex_t *mutex);

void mutex_destory(mutex_t *mutex);

int sem_init(sem_t *sem, int init_count);

void sem_wait(sem_t *sem);

int sem_trywait(sem_t *sem);

int sem_timedwait(sem_t *sem, tm_t *tmo);

void sem_notify(sem_t *sem);

int sem_count(sem_t *sem);

int get_clocktime(tm_t *time);

time_t mktime(tm_t *time);

int local_time(tm_t *tm, time_t time);
#endif
