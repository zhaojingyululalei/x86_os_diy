#include "string.h"
#include "types.h"
#include <stdarg.h>

// 字符串拷贝函数 strcpy
void strcpy(char *dest, const char *src) {
    while ((*dest++ = *src++));
}

// 有限长度字符串拷贝函数 strncpy
void strncpy(char *dest, const char *src, int size) {
    int i;
    for (i = 0; i < size && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < size; i++) {
        dest[i] = '\0';
    }
}

// 有限长度字符串比较函数 strncmp
int strncmp(const char *s1, const char *s2, int size) {
    for (int i = 0; i < size; i++) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

// 字符串长度函数 strlen
int strlen(const char *str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

// 内存拷贝函数 memcpy
void memcpy(void *dest, const void *src, int size) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    for (int i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

// 内存填充函数 memset
void memset(void *dest, uint8_t v, int size) {
    uint8_t *d = dest;
    for (int i = 0; i < size; i++) {
        d[i] = v;
    }
}

// 内存比较函数 memcmp
int memcmp(const void *d1, const void *d2, int size) {
    const uint8_t *s1 = d1;
    const uint8_t *s2 = d2;
    for (int i = 0; i < size; i++) {
        if (s1[i] != s2[i]) {
            return s1[i] - s2[i];
        }
    }
    return 0;
}

// 整数转字符串函数 itoa
void itoa(char *buf, int num, int base) {
    char *ptr = buf;
    int is_negative = (num < 0 && base == 10);

    if (is_negative) {
        num = -num;
    }

    int temp = num;
    do {
        int remainder = temp % base;
        *ptr++ = (remainder > 9) ? (remainder - 10) + 'a' : remainder + '0';
    } while (temp /= base);

    if (is_negative) {
        *ptr++ = '-';
    }

    *ptr = '\0';

    // Reverse the string
    char *start = buf;
    char *end = ptr - 1;
    while (start < end) {
        char tmp = *start;
        *start++ = *end;
        *end-- = tmp;
    }
}

// 格式化字符串输出函数 sprintf
void sprintf(char *buffer, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);
}

// 格式化字符串输出函数 vsprintf
void vsprintf(char *buffer, const char *fmt, va_list args) {
    char *buf_ptr = buffer;
    const char *fmt_ptr = fmt;
    while (*fmt_ptr) {
        if (*fmt_ptr == '%' && *(fmt_ptr + 1) != '%') {
            fmt_ptr++;
            switch (*fmt_ptr) {
                case 'd': {
                    int num = va_arg(args, int);
                    itoa(buf_ptr, num, 10);
                    buf_ptr += strlen(buf_ptr);
                    break;
                }
                case 'x': {
                    int num = va_arg(args, int);
                    itoa(buf_ptr, num, 16);
                    buf_ptr += strlen(buf_ptr);
                    break;
                }
                case 's': {
                    const char *str = va_arg(args, const char *);
                    strcpy(buf_ptr, str);
                    buf_ptr += strlen(str);
                    break;
                }
                default:
                    *buf_ptr++ = *fmt_ptr;
                    break;
            }
        } else {
            *buf_ptr++ = *fmt_ptr;
        }
        fmt_ptr++;
    }
    *buf_ptr = '\0';
}


