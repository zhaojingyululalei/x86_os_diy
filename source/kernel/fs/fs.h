#ifndef __FS_H
#define __FS_H
#include "string.h"
#include "cpu_cfg.h"
#include "cpu_instr.h"

int sys_open(const char* path,int flags,...);
int sys_read(int fd,char* buf,int len);
int sys_write(int fd,const char* buf,int len);
int sys_lseek(int fd,int offset, int whence);
int sys_close(int fd);
#endif
