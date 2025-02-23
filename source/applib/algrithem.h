#ifndef __ALGRITHEM_H
#define __ALGRITHEM_H
#include "types.h"
// 线性同余法参数（这些常数可以根据需要调整）
#define A 1664525
#define C 1013904223
#define M 4294967296  // 2^32

// 随机数生成器结构体，保存当前的种子值
typedef struct {
    unsigned int seed;
} randam_generator_t;

uint16_t checksum16(uint32_t offset, void* buf, uint16_t len, uint32_t pre_sum, int complement) ;

void init_random(randam_generator_t *rg, unsigned int seed);
uint32_t get_random(randam_generator_t *rg);
uint32_t random(uint32_t time_seed);
#endif