
#include "math.h"

// 向上对齐
uint32_t align_up_to(uint32_t from,uint32_t aligment) {
    return (from + (aligment - 1)) & ~(aligment - 1);
}

// 向下对齐
uint32_t align_down_to(uint32_t from,uint32_t aligment) {
    return from & ~(aligment - 1);
}