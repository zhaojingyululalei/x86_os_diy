#ifndef __SERIAL_H
#define __SERIAL_H

#include "cpu.h"
#include "string.h"
#include "cpu_instr.h"
#include <stdarg.h>

void rs232_init(void);
void serial_printf(const char * str_buf);
#endif