#ifndef __TIME_H
#define __TIME_H
#include "types.h"
typedef uint32_t time_t;
typedef struct _tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
}tm_t;

time_t kernel_mktime(tm_t *tm);
int localtime(tm_t *tm, time_t time);
time_t sys_mktime(tm_t* tm);
int sys_local_time(tm_t *tm, time_t time);
uint32_t get_time_seed(void);
struct timeval{
    int tv_sec;
    int tv_usec;
};
#endif
