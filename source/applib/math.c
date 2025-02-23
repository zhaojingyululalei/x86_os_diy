
#include "math.h"

// 向上对齐
uint32_t align_up_to(uint32_t from,uint32_t aligment) {
    return (from + (aligment - 1)) & ~(aligment - 1);
}

// 向下对齐
uint32_t align_down_to(uint32_t from,uint32_t aligment) {
    return from & ~(aligment - 1);
}

int power(int num, int x) {
    if (x == 0) {
        return 1;
    }

   
    if (x < 0) {
        return 1 / power(num, -x);
    }

    // 如果指数是奇数或偶数的情况
    if (x % 2 == 0) {
        int half = power(num, x / 2);  
        return half * half;  
    } else {
        return num * power(num, x - 1);  
    }
}