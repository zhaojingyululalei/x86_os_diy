#include "_stdio.h"
#include "_stdlib.h"


char getchar(){
    char c;
    read(stdin,&c,1);
    return c;
}

int putchar(char c){
    return write(stdout,&c,1);

}

int fgets(int fd,char* buf,int len){
    int ret;
    ret = read(fd,buf,len);
    buf[len-1] = '\0';
    return ret;
}
int fputs(int fd,const char* buf,int len){
    int ret;
    ret = write(fd,buf,len);
    return ret;
}

