#include "time.h"

// 时间单位定义
#define MINUTE 60
#define HOUR (60 * MINUTE)
#define DAY (24 * HOUR)
#define YEAR (365 * DAY)
#define LEAP_YEAR_DAY (366 * DAY)

// 每月天数定义（默认 2 月为 28 天）
static const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// 判断是否为闰年
static int is_leap_year(int year)
{
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

/**
 * 将本地时间转换为时间戳
 */
time_t kernel_mktime(tm_t *tm)
{
    int year = tm->tm_year;
    long days = 0;

    // 处理 Y2K 问题
    if (year < 70)
        year += 100; // 把 00-69 解析为 2000-2069
    year += 1900;    // 得到实际年份

    // 累加从 1970 年到指定年份的天数
    for (int y = 1970; y < year; y++)
    {
        days += is_leap_year(y) ? 366 : 365;
    }

    // 累加当年月份的天数
    for (int m = 0; m < tm->tm_mon; m++)
    {
        days += days_in_month[m];
        if (m == 1 && is_leap_year(year))
        { // 2 月在闰年多 1 天
            days += 1;
        }
    }

    // 累加当月天数
    days += tm->tm_mday - 1;

    // 计算总秒数
    return days * DAY + tm->tm_hour * HOUR + tm->tm_min * MINUTE + tm->tm_sec;
}

/**
 * 计算时间戳对应的本地时间
 */
int localtime(tm_t *tm, time_t time)
{
    if (tm == NULL)
        return -1;

    int year = 1970;
    int days = time / DAY;
    int remaining_secs = time % DAY;

    // 解析小时、分钟、秒
    tm->tm_hour = remaining_secs / HOUR;
    remaining_secs %= HOUR;
    tm->tm_min = remaining_secs / MINUTE;
    tm->tm_sec = remaining_secs % MINUTE;

    // 计算年份
    while (days >= (is_leap_year(year) ? 366 : 365))
    {
        days -= is_leap_year(year) ? 366 : 365;
        year++;
    }
    tm->tm_year = year - 1900; // 转换为 1900 年的偏移
    tm->tm_year = tm->tm_year >= 100 ? tm->tm_year - 100 : tm->tm_year;
    // 计算一年中的第几天
    tm->tm_yday = days;

    // 计算月份和日期
    int month = 0;
    while (days >= days_in_month[month] + (month == 1 && is_leap_year(year) ? 1 : 0))
    {
        days -= days_in_month[month] + (month == 1 && is_leap_year(year) ? 1 : 0);
        month++;
    }
    tm->tm_mon = month;
    tm->tm_mday = days + 1;

    // 计算星期几（1970-01-01 是星期四）
    tm->tm_wday = (time / DAY + 4) % 7;

    // 假设不使用夏令时
    tm->tm_isdst = 0;

    return 0;
}