#ifndef __STDLIB_H
#define __STDLIB_H
#include "types.h"
typedef struct _syscall_args_t {
    int id;
    int arg0;
    int arg1;
    int arg2;
    int arg3;
}syscall_args_t;

int sleep(int ms);
int getpid(void);
void printf_tmp(char* fmt, int arg);
int fork(void);
int execve(const char *path, char * const *argv, char * const *env) ;
int yield(void);
void exit(int status);
int wait(int* status);
#endif

