#ifndef __SYSCALL_H
#define __SYSCALL_H

/**
 * 系统调用的栈信息
 */
typedef struct _syscall_frame_t {
	int eflags;
	int gs, fs, es, ds;
	int edi, esi, ebp, dummy, ebx, edx, ecx, eax;
	int eip, cs;
	int func_id, arg0, arg1, arg2, arg3, arg4, arg5;
	int esp, ss;
}syscall_frame_t;



#endif

