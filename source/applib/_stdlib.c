#include "_stdlib.h"
#include "debug.h"
#include "syscall_table.h"
#include "../os_config/cpu_cfg.h"
#include "_time.h"
#include "../lib/string.h"
#include "net/socket.h"
#include "fs/fs.h"
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
    __asm__ __volatile__ (
            "mov %[args_base], %%eax\r\n"
            "push 24(%%eax)\n\t"             // arg5
            "push 20(%%eax)\n\t"             // arg4
            "push 16(%%eax)\n\t"             // arg3
            "push 12(%%eax)\n\t"             // arg2
            "push 8(%%eax)\n\t"              // arg1
            "push 4(%%eax)\n\t"              // arg0
            "push 0(%%eax)\n\t"              // id
            "lcalll *(%[gate])\n\n"
            :"=a"(ret)
            :[args_base]"r"(args), [gate]"r"(sys_gate_addr));
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

void ser_printf(const char* str){
    syscall_args_t args;
    args.id= SYS_SER_printf;
    args.arg0 = (int)str;
    sys_call(&args);
}
void printf_inner(const char *str)
{
#ifdef DBG_OUTPUT_SERIAL
    ser_printf(str);
#elif defined(DBG_OUTPUT_TTY)
    write(1,str,strlen(str));
#else
    
#endif
}
void printf(char *fmt, ...)
{
    char str_buf[MAX_STR_BUF_SIZE];
    va_list args;
    int offset = 0;
    // 清空缓冲区
    memset(str_buf, '\0', sizeof(str_buf));

    // 格式化日志信息
    va_start(args, fmt);
    vsprintf(str_buf + offset, fmt, args);
    va_end(args);
    //printf_inner(str_buf);
    write(1,str_buf,strlen(str_buf));
}

int fork(void) {
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
    syscall_args_t args;
    args.id = SYS_wait;
    args.arg0 = (int)status;
    return sys_call(&args);
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
    return sys_call(&args);
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


int socket(int family, int type, int protocol) {
    syscall_args_t args;
    args.id = SYS_socket;
    args.arg0 = (int)family;
    args.arg1 = (int)type;
    args.arg2 = (int)protocol;
    return sys_call(&args);
}

int closesocket(int sockfd) {
    syscall_args_t args;
    args.id = SYS_closesocket;
    args.arg0 = (int)sockfd;
    return sys_call(&args);
}

int listen(int sockfd, int backlog) {
    syscall_args_t args;
    args.id = SYS_listen;
    args.arg0 = (int)sockfd;
    args.arg1 = (int)backlog;
    return sys_call(&args);
}

int accept(int sockfd, struct sockaddr* addr, socklen_t* len) {
    syscall_args_t args;
    args.id = SYS_accept;
    args.arg0 = (int)sockfd;
    args.arg1 = (int)addr;
    args.arg2 = (int)len;
    return sys_call(&args);
}

ssize_t sendto(int sockfd, const void* buf, ssize_t len, int flags, const struct sockaddr* dest, socklen_t dest_len) {
    syscall_args_t args;
    args.id = SYS_sendto;
    args.arg0 = (int)sockfd;
    args.arg1 = (int)buf;
    args.arg2 = (int)len;
    args.arg3 = (int)flags;
    args.arg4 = (int)dest;
    args.arg5 = (int)dest_len;
    return (ssize_t)sys_call(&args);
}

ssize_t recvfrom(int sockfd, void* buf, ssize_t len, int flags, struct sockaddr* src, socklen_t* src_len) {
    syscall_args_t args;
    args.id = SYS_recvfrom;
    args.arg0 = (int)sockfd;
    args.arg1 = (int)buf;
    args.arg2 = (int)len;
    args.arg3 = (int)flags;
    args.arg4 = (int)src;
    args.arg5 = (int)src_len;
    return (ssize_t)sys_call(&args);    
}

int connect(int sockfd, const struct sockaddr* addr, socklen_t len) {
    syscall_args_t args;
    args.id = SYS_connect;
    args.arg0 = (int)sockfd;
    args.arg1 = (int)addr;
    args.arg2 = (int)len;
    return (ssize_t)sys_call(&args); 
}

int bind(int sockfd, const struct sockaddr* addr, socklen_t len) {
    syscall_args_t args;
    args.id = SYS_bind;
    args.arg0 = (int)sockfd;
    args.arg1 = (int)addr;
    args.arg2 = (int)len;
    return (ssize_t)sys_call(&args); 
}

ssize_t send(int sockfd, const void* buf, ssize_t len, int flags) {
    syscall_args_t args;
    args.id = SYS_send;
    args.arg0 = (int)sockfd;
    args.arg1 = (int)buf;
    args.arg2 = (int)len;
    args.arg3 = (int)flags;
    return (ssize_t)sys_call(&args); 
}

ssize_t recv(int sockfd, void* buf, ssize_t len, int flags) {
    syscall_args_t args;
    args.id = SYS_recv;
    args.arg0 = (int)sockfd;
    args.arg1 = (int)buf;
    args.arg2 = (int)len;
    args.arg3 = (int)flags;
    return (ssize_t)sys_call(&args); 
}

int setsockopt(int sockfd, int level, int optname, const char * optval, int optlen) {
    syscall_args_t args;
    args.id = SYS_setsockopt;
    args.arg0 = (int)sockfd;
    args.arg1 = (int)level;
    args.arg2 = (int)optname;
    args.arg3 = (int)optval;
    args.arg4 = (int)optlen;
    return sys_call(&args); 
}

int open(const char* path,int flags,mode_t mode){
    syscall_args_t args;
    args.id = SYS_open;
    args.arg0 = (int)path;
    args.arg1 = flags;
    args.arg2 = mode;
    return sys_call(&args); 
}
int read(int fd,char* buf,int len){
    syscall_args_t args;
    args.id = SYS_read;
    args.arg0 = fd;
    args.arg1 = (int)buf;
    args.arg2 = len;
    return sys_call(&args); 
}
int write(int fd,const char* buf,int len){
    syscall_args_t args;
    args.id = SYS_write;
    args.arg0 = fd;
    args.arg1 = (int)buf;
    args.arg2 = len;
    return sys_call(&args); 
}
int lseek(int fd,int offset, int whence){
    syscall_args_t args;
    args.id = SYS_lseek;
    args.arg0 = fd;
    args.arg1 = offset;
    args.arg2 = whence;
    return sys_call(&args); 
}
int close(int fd){
    syscall_args_t args;
    args.id = SYS_close;
    args.arg0 = fd;
    return sys_call(&args); 
}
int dup(int fd){
    syscall_args_t args;
    args.id = SYS_dup;
    args.arg0 = fd;
    return sys_call(&args);
}























// 32位无符号整数转换函数（主机到网络）
uint32_t htonl(uint32_t hostlong)
{
    if (IS_LITTLE_ENDIAN)
    {
        return ((hostlong >> 24) & 0x000000ff) |
               ((hostlong >> 8) & 0x0000ff00) |
               ((hostlong << 8) & 0x00ff0000) |
               ((hostlong << 24) & 0xff000000);
    }
    return hostlong; // 如果系统是大端字节序，则无需转换
}

// 16位无符号整数转换函数（主机到网络）
uint16_t htons(uint16_t hostshort)
{
    if (IS_LITTLE_ENDIAN)
    {
        return ((hostshort >> 8) & 0x00ff) |
               ((hostshort << 8) & 0xff00);
    }
    return hostshort;
}

// 32位无符号整数转换函数（网络到主机）
uint32_t ntohl(uint32_t netlong)
{
    if (IS_LITTLE_ENDIAN)
    {
        return ((netlong >> 24) & 0x000000ff) |
               ((netlong >> 8) & 0x0000ff00) |
               ((netlong << 8) & 0x00ff0000) |
               ((netlong << 24) & 0xff000000);
    }
    return netlong;
}

// 16位无符号整数转换函数（网络到主机）
uint16_t ntohs(uint16_t netshort)
{
    if (IS_LITTLE_ENDIAN)
    {
        return ((netshort >> 8) & 0x00ff) |
               ((netshort << 8) & 0xff00);
    }
    return netshort;
}

int inet_pton(int family, const char *strptr, void *addrptr)
{
    return 0;
}
const char * inet_ntop(int family, const void *addrptr, char *strptr, int len)
{
    return NULL;
}