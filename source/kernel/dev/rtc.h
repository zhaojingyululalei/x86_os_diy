#ifndef __RTC_H
#define __RTC_H


#include "_time.h"
extern time_t startup_time;
void rtc_init(void);
int sys_get_clocktime(tm_t* time);
#endif
