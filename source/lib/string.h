#ifndef __KLIB_H
#define __KLIB_H

#include "types.h"
#include <stdarg.h>

void strcpy (char * dest, const char * src);
void strncpy(char * dest, const char * src, int size);
int strncmp (const char * s1, const char * s2, int size);
int  strlen(const char * str);
void memcpy (void * dest, const void * src, int size);
void memset(void * dest, uint8_t v, int size);
int memcmp (const void * d1, const void * d2, int size);
void itoa(char * buf, int num, int base);
void sprintf(char * buffer, const char * fmt, ...);
void vsprintf(char * buffer, const char * fmt, va_list args);



#endif