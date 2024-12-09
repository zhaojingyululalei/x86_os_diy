#ifndef __MATH_H
#define __MATH_H

#include "types.h"

// 向上对齐
uint32_t align_up_to(uint32_t from,uint32_t aligment);

// 向下对齐
uint32_t align_down_to(uint32_t from,uint32_t aligment);

//平方
int power(int num, int x);
#endif
