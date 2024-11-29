#include "cpu_instr.h"
#include "rtc.h"
#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

// 使用 inb 和 outb 函数从 CMOS 读取指定寄存器的值
static inline uint8_t read_cmos(uint8_t reg) {
    outb(CMOS_ADDRESS, reg);          // 设置寄存器地址
    return inb(CMOS_DATA);            // 从 CMOS 数据端口读取数据
}

// BCD to Binary 转换宏
#define BCD_TO_BIN(val) ((val) = ((val) & 0x0F) + ((val) >> 4) * 10)

time_t startup_time;

void rtc_init(void)
{
    tm_t time;

    do {
        time.tm_sec = read_cmos(0x00);    // 秒
        time.tm_min = read_cmos(0x02);    // 分钟
        time.tm_hour = read_cmos(0x04);   // 小时
        time.tm_mday = read_cmos(0x07);   // 日
        time.tm_mon = read_cmos(0x08);    // 月
        time.tm_year = read_cmos(0x09);   // 年
    } while (time.tm_sec != read_cmos(0x00));  // 再次读取秒，确保数据一致性

    // 将 BCD 编码转换为二进制
    BCD_TO_BIN(time.tm_sec);
    BCD_TO_BIN(time.tm_min);
    BCD_TO_BIN(time.tm_hour);
    BCD_TO_BIN(time.tm_mday);
    BCD_TO_BIN(time.tm_mon);
    BCD_TO_BIN(time.tm_year);

    time.tm_mon--;  // 月份从 0 开始
    startup_time = kernel_mktime(&time);  // 将时间转换为 UNIX 时间戳
    //localtime(&test,startup_time);
    return;
    
}