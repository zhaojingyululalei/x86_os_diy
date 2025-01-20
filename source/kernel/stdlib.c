#include "_stdlib.h"
#include "syscall_table.h"
#include "cpu_cfg.h"
#include "ipc/mutex.h"
#include "ipc/semaphor.h"
/**
 * 直接-L -l的话，这些函数重定位不在0x80000000以上范围内。而是在内核区。用户掉不了。
 */
/**
 * 执行系统调用
 */
static inline int sys_call (syscall_args_t * args) {
    //lcalll选择子即可，不需要偏移量
    const unsigned long sys_gate_addr[] = {0, SELECTOR_CALL_GATE};  
    int ret;

    // 采用调用门, 这里只支持5个参数
    // 用调用门的好处是会自动将参数复制到内核栈中，这样内核代码很好取参数
    // 而如果采用寄存器传递，取参比较困难，需要先压栈再取
    __asm__ __volatile__(
            "push %[arg3]\n\t"
            "push %[arg2]\n\t"
            "push %[arg1]\n\t"
            "push %[arg0]\n\t"
            "push %[id]\n\t"
            "lcalll *(%[gate])\n\n"
            :"=a"(ret)
            :[arg3]"r"(args->arg3), [arg2]"r"(args->arg2), [arg1]"r"(args->arg1),
    [arg0]"r"(args->arg0), [id]"r"(args->id),
    [gate]"r"(sys_gate_addr));
    return ret;
}

int sleep(int ms)
{
    if (ms <= 0) {
        return -1;
    }

    syscall_args_t args;
    args.id = SYS_msleep;
    args.arg0 = ms;
    return sys_call(&args);
}

int getpid(void) {
    syscall_args_t args;
    args.id = SYS_getpid;
    return sys_call(&args);
}

/*临时使用的，用来调试*/
void printf_tmp(char* fmt, int arg)
{
    syscall_args_t args;
    args.id = SYS_printf_tmp;
    args.arg0 = (int)fmt;
    args.arg1 = arg;
    sys_call(&args);
}

int fork() {
    syscall_args_t args;
    args.id = SYS_fork;
    return sys_call(&args);
}

int execve(const char *path, char * const *argv, char * const *env) {
    syscall_args_t args;
    args.id = SYS_execve;
    args.arg0 = (int)path;
    args.arg1 = (int)argv;
    args.arg2 = (int)env;
    return sys_call(&args);
}

int yield(void)
{
    syscall_args_t args;
    args.id  = SYS_yield;
    return sys_call(&args);
}

void exit(int status)
{
    syscall_args_t args;
    args.id = SYS_exit;
    args.arg0 = (int)status;
    sys_call(&args);
    //其实也不会运行到这里来，因为系统调用把该进程变成僵尸进程，没有被调度的机会了
    while(1)
    {
        ;
    }
}

int wait(int* status) {
    int ret;
    syscall_args_t args;
    args.id = SYS_wait;
    args.arg0 = (int)status;
    ret = sys_call(&args);
    return ret;
}
void* malloc(int size)
{
    syscall_args_t args;
    args.id = SYS_malloc;
    args.arg0 = size;
    return sys_call(&args);
}

void free(void* ptr)
{
    syscall_args_t args;
    args.id = SYS_free;
    args.arg0 = ptr;
   sys_call(&args);
}


void mutex_init(mutex_t * mutex)
{
    syscall_args_t args;
    args.id = SYS_mutex_init;
    args.arg0 = mutex;
   sys_call(&args);
}

void mutex_lock(mutex_t * mutex)
{
    syscall_args_t args;
    args.id = SYS_mutex_lock;
    args.arg0 = mutex;
    sys_call(&args);
}

void mutex_unlock(mutex_t* mutex)
{
    syscall_args_t args;
    args.id = SYS_mutex_unlock;
    args.arg0 = mutex;
    sys_call(&args);
}

void mutex_destory(mutex_t* mutex)
{
    syscall_args_t args;
    args.id = SYS_mutex_destory;
    args.arg0 = mutex;
    sys_call(&args);
}

int sem_init(sem_t* sem,int init_count)
{
    syscall_args_t args;
    args.id = SYS_sem_init;
    args.arg0 = sem;
    args.arg1 = init_count;
    return sys_call(&args);
}

void sem_wait(sem_t* sem)
{
    syscall_args_t args;
    args.id = SYS_sem_wait;
    args.arg0 = sem;
    sys_call(&args);
}

int sem_trywait(sem_t* sem)
{
    syscall_args_t args;
    args.id = SYS_sem_trywait;
    args.arg0 = sem;
    return sys_call(&args);
}

int sem_timedwait(sem_t* sem,tm_t* tmo)
{
    syscall_args_t args;
    args.id = SYS_sem_timedwait;
    args.arg0 = sem;
    args.arg1 = tmo;
    return sys_call(&args);
}

void sem_notify(sem_t* sem)
{
    syscall_args_t args;
    args.id = SYS_sem_notify;
    args.arg0 = sem;
    sys_call(&args);
}

int sem_count(sem_t* sem)
{
    syscall_args_t args;
    args.id = SYS_sem_count;
    args.arg0 = sem;
    return sys_call(&args);
}

int get_clocktime(tm_t* time)
{
    syscall_args_t args;
    args.id = SYS_get_clocktime;
    args.arg0 = time;
    return sys_call(&args);
}

time_t mktime(tm_t* time)
{
    syscall_args_t args;
    args.id = SYS_mktime;
    args.arg0 = time;
    return sys_call(&args);
} 

int local_time(tm_t *tm, time_t time)
{
    syscall_args_t args;
    args.id = SYS_local_time;
    args.arg0 = tm;
    args.arg1 = time;

    return sys_call(&args);
}

