#ifndef __ERR_H
#define __ERR_H
#include "debug.h"
#define MAX_ERR_STR_NR 1024
typedef enum _os_error_t
{
    ERROR_OK=0,
    TASK_NULL,
    DESC_TSS_CROSS_BORDER,
    MEM_NOT_ENOUGH,


}os_error_t;

static char* err_str[MAX_ERR_STR_NR]=
{
    [ERROR_OK] = "everything is ok",
    [TASK_NULL] = "there is an empty task",
    [DESC_TSS_CROSS_BORDER] = "tss desc cross gdt border",
    [MEM_NOT_ENOUGH] = "out of memory",
};

#endif
