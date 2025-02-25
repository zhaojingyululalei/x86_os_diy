#ifndef ___STDIO_H
#define ___STDIO_H

char getchar();
int putchar(char c);
int fgets(int fd,char* buf,int len);
int fputs(int fd,const char* buf,int len);

#endif