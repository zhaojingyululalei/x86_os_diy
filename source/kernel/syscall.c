#include "types.h"
#include "syscall.h"
#include "debug.h"
#include "syscall_table.h"
#include "task/task.h"
#include "ipc/mutex.h"
#include "ipc/semaphor.h"
#include "rtc.h"
#include "chr/serial.h"
#include "console.h"
#include "net/socket.h"
#include "fs/fs.h"
#include "dev.h"
// 系统调用处理函数类型
typedef int (*syscall_handler_t)(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);

int sys_printf_tmp(char *fmt, int arg)
{
	char str_buf[MAX_STR_BUF_SIZE] = {0};
	sprintf(str_buf, fmt, arg);
#ifdef DBG_OUTPUT_SERIAL
	serial_printf(str_buf);
#elif defined(DBG_OUTPUT_TTY)
	dev_write(0,0,str_buf,strlen(str_buf));
#else

#endif
}

// 系统调用表
static const syscall_handler_t sys_table[] = {
	[SYS_msleep] = (syscall_handler_t)sys_sleep_ms,
	[SYS_getpid] = (syscall_handler_t)sys_getpid,
	[SYS_printf_tmp] = (syscall_handler_t)sys_printf_tmp,
	[SYS_fork] = (syscall_handler_t)sys_fork,
	[SYS_execve] = (syscall_handler_t)sys_execve,
	[SYS_yield] = (syscall_handler_t)sys_yield,
	[SYS_exit] = (syscall_handler_t)sys_exit,
	[SYS_wait] = (syscall_handler_t)sys_wait,
	[SYS_malloc] = (syscall_handler_t)sys_malloc,
	[SYS_free] = (syscall_handler_t)sys_free,
	[SYS_mutex_init] = (syscall_handler_t)sys_mutex_init,
	[SYS_mutex_lock] = (syscall_handler_t)sys_mutex_lock,
	[SYS_mutex_unlock] = (syscall_handler_t)sys_mutex_unlock,
	[SYS_mutex_destory] = (syscall_handler_t)sys_mutex_destory,
	[SYS_sem_init] = (syscall_handler_t)sys_sem_init,
	[SYS_sem_wait] = (syscall_handler_t)sys_sem_wait,
	[SYS_sem_trywait] = (syscall_handler_t)sys_sem_trywait,
	[SYS_sem_timedwait] = (syscall_handler_t)sys_sem_timedwait,
	[SYS_sem_notify] = (syscall_handler_t)sys_sem_notify,
	[SYS_sem_count] = (syscall_handler_t)sys_sem_count,
	[SYS_get_clocktime] = (syscall_handler_t)sys_get_clocktime,
	[SYS_mktime] = (syscall_handler_t)sys_mktime,
	[SYS_local_time] = (syscall_handler_t)sys_local_time,
	[SYS_SER_printf] = (syscall_handler_t)sys_serial_printf,
	[SYS_socket] = (syscall_handler_t)sys_socket,
	[SYS_closesocket] = (syscall_handler_t)sys_closesocket,
	[SYS_listen] = (syscall_handler_t)sys_listen,
	[SYS_accept] = (syscall_handler_t)sys_accept,
	[SYS_sendto] = (syscall_handler_t)sys_sendto,
	[SYS_recvfrom] = (syscall_handler_t)sys_recvfrom,
	[SYS_connect] = (syscall_handler_t)sys_connect,
	[SYS_bind] = (syscall_handler_t)sys_bind,
	[SYS_send] = (syscall_handler_t)sys_send,
	[SYS_recv] = (syscall_handler_t)sys_recv,
	[SYS_setsockopt] = (syscall_handler_t)sys_setsockopt,
	[SYS_open] = (syscall_handler_t)sys_open,
	[SYS_read] = (syscall_handler_t)sys_read,
	[SYS_write] = (syscall_handler_t)sys_write,
	[SYS_lseek] = (syscall_handler_t)sys_lseek,
	[SYS_close] = (syscall_handler_t)sys_close,
	[SYS_dup] = (syscall_handler_t)sys_dup,
};

/**
 * 处理系统调用。该函数由系统调用函数调用
 */
void do_handler_syscall(syscall_frame_t *frame)
{
	// 超出边界，返回错误
	if (frame->func_id < sizeof(sys_table) / sizeof(sys_table[0]))
	{
		// 查表取得处理函数，然后调用处理
		syscall_handler_t handler = sys_table[frame->func_id];
		if (handler)
		{
			int ret = handler(frame->arg0, frame->arg1, frame->arg2, frame->arg3, frame->arg4, frame->arg5);
			frame->eax = ret; // 设置系统调用的返回值，由eax传递
			return;
		}
	}

	// 不支持的系统调用，打印出错信息
	task_t *task = get_cur_task();
	dbg_error("task pid:%d,Unknown syscall: %d", task->pid, frame->func_id);
	frame->eax = -1; // 设置系统调用的返回值，由eax传递
}
