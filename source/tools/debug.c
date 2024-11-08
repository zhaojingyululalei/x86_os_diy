#include "debug.h"
#include "chr/serial.h"
#include "cpu_instr.h"

#define MAX_STR_BUF_SIZE 256
/**
 * @brief 格式化日志并打印到串口（使用 sprintf 和 vsprintf）
 */
void dbg_print(int level,const char *file, const char *func, int line, const char *fmt, ...) {
    if(level > DBG_LEVEL_CTL_SET)
    {
        return;
    }
    char str_buf[MAX_STR_BUF_SIZE];
    va_list args;

    // 清空缓冲区
    memset(str_buf, '\0', sizeof(str_buf));

    // 组装文件、函数和行号信息
    sprintf(str_buf, "in file:%s, func:%s, line:%d: ", file, func, line);
    int offset = strlen(str_buf);

    // 格式化日志信息
    va_start(args, fmt);
    vsprintf(str_buf + offset, fmt, args);
    va_end(args);

    // 检查调试输出标志是否启用
    #ifdef DBG_OUTPUT_SERIAL
        serial_printf(str_buf);
    #elif DBG_OUTPUT_TTY

    #else

    #endif
}


void panic (const char * file, int line, const char * func, const char * cond) {
    char str_buf[MAX_STR_BUF_SIZE];

    sprintf(str_buf, "in file:%s, func:%s, line:%d: %s", file, func, line,cond);
    // 检查调试输出标志是否启用
    #ifdef DBG_OUTPUT_SERIAL
        serial_printf(str_buf);
    #elif DBG_OUTPUT_TTY

    #else

    #endif
    for (;;) {
        hlt();
    }
}