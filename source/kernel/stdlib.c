#include "stdlib.h"
#include "syscall_table.h"
#include "cpu_cfg.h"
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
    return sys_call(&args);
}