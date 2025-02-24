#ifndef ___STDLIB_H
#define ___STDLIB_H
#include "types.h"
#include "../kernel/ipc/mutex.h"
#include "../kernel/ipc/semaphor.h"
#include "fs/fs.h"
// 判断当前系统是否是小端,0xff在低字节还是高字节
#define IS_LITTLE_ENDIAN (*(uint16_t *)"\0\xff" > 0x100)
typedef struct _syscall_args_t
{
    int id;
    int arg0;
    int arg1;
    int arg2;
    int arg3;
    int arg4;
    int arg5;
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
void printf(char *fmt, ...);

int open(const char* path,int flags,mode_t mode);
int read(int fd,char* buf,int len);
int write(int fd,const char* buf,int len);
int lseek(int fd,int offset, int whence);
int close(int fd);
int dup(int fd);


// 32位无符号整数转换函数（主机到网络）
uint32_t htonl(uint32_t hostlong);
// 16位无符号整数转换函数（主机到网络）
uint16_t htons(uint16_t hostshort);
// 32位无符号整数转换函数（网络到主机）
uint32_t ntohl(uint32_t netlong);
// 16位无符号整数转换函数（网络到主机）
uint16_t ntohs(uint16_t netshort);
int inet_pton(int family, const char *strptr, void *addrptr);
const char * inet_ntop(int family, const void *addrptr, char *strptr, int len);
#endif
