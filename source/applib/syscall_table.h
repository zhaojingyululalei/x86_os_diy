#ifndef __SYS_CALL_TABLE_H
#define __SYS_CALL_TABLE_H

#define SYS_msleep 0
#define SYS_getpid 1
#define SYS_printf_tmp 2
#define SYS_fork 3
#define SYS_execve 4
#define SYS_yield   5
#define SYS_exit 6
#define SYS_wait 7
#define SYS_malloc 8
#define SYS_free 9
#define SYS_mutex_init 10
#define SYS_mutex_lock 11
#define SYS_mutex_unlock 12
#define SYS_mutex_destory 13
#define SYS_sem_init 14
#define SYS_sem_wait 15
#define SYS_sem_trywait 16
#define SYS_sem_timedwait 17
#define SYS_sem_notify 18
#define SYS_sem_count 19
#define SYS_get_clocktime 20
#define SYS_mktime 21
#define SYS_local_time 22
#define SYS_SER_printf 23
#define SYS_socket  24
#define SYS_closesocket 25
#define SYS_listen 26
#define SYS_accept 27
#define SYS_sendto 28
#define SYS_recvfrom 29
#define SYS_connect 30
#define SYS_bind 31
#define SYS_send 32
#define SYS_recv 33
#define SYS_setsockopt 34
#define SYS_open 35
#define SYS_read 36
#define SYS_write 37
#define SYS_lseek 38
#define SYS_close 39
#endif

