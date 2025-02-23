#include "debug.h"
#include "chr/serial.h"
#include "cpu_instr.h"
#include "console.h"

/**
 * @brief 格式化日志并打印到串口
 */
void dbg_print(int level, const char *file, const char *func, int line, const char *fmt, ...)
{
    if (level > DBG_LEVEL_CTL_SET)
    {
        return;
    }

    static const char *title[] = {
        [DBG_LEVEL_ERROR] = "error",
        [DBG_LEVEL_WARNING] = "warning",
        [DBG_LEVEL_INFO] = "info",
        [DBG_LEVEL_NONE] = "none"};

    char str_buf[MAX_STR_BUF_SIZE];
    va_list args;
    int offset=0;
    // 清空缓冲区
    memset(str_buf, '\0', sizeof(str_buf));

    // 组装文件、函数和行号信息
    if (level != DBG_LEVEL_INFO)
    {
        sprintf(str_buf, "[%s] in file:%s, func:%s, line:%d: \r\n", title[level], file, func, line);
        offset = strlen(str_buf);
    }

    // 格式化日志信息
    va_start(args, fmt);
    vsprintf(str_buf + offset, fmt, args);
    va_end(args);

// 检查调试输出标志是否启用
#ifdef DBG_OUTPUT_SERIAL
    serial_printf(str_buf);
#elif defined(DBG_OUTPUT_TTY)
    console_write(0, str_buf, strlen(str_buf));
#else
    
#endif
}

void panic(const char *file, int line, const char *func, const char *cond)
{
    char str_buf[MAX_STR_BUF_SIZE];

    sprintf(str_buf, "[assert] in file:%s, func:%s, line:%d: %s", file, func, line, cond);
// 检查调试输出标志是否启用
#ifdef DBG_OUTPUT_SERIAL
    serial_printf(str_buf);
#elif defined(DBG_OUTPUT_TTY)
    console_write(0, str_buf, strlen(str_buf));
#else
    
#endif
    for (;;)
    {
        hlt();
    }
}