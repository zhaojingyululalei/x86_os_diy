#include "algrithem.h"
#include "_time.h"
#include "rtc.h"
/**
 * 计算16位校验和
 * @param buf 数组缓存区起始
 * @param len 数据长
 * @param 之前对其它缓存进行累加的结果计数，用于方便计算伪首部校验和
 */
uint16_t checksum16(uint32_t offset, void* buf, uint16_t len, uint32_t pre_sum, int complement) 
{
    uint16_t* curr_buf = (uint16_t *)buf;
    uint32_t checksum = pre_sum;

    // 起始字节不对齐, 加到高8位
    if (offset & 0x1) {
        uint8_t * buf = (uint8_t *)curr_buf;
        checksum += *buf++ << 8;
        curr_buf = (uint16_t *)buf;
        len--;
    }

    while (len > 1) {
        checksum += *curr_buf++;
        len -= 2;
    }

    if (len > 0) {
        checksum += *(uint8_t*)curr_buf;
    }

    // 注意，这里要不断累加。不然结果在某些情况下计算不正确
    uint16_t high;
    while ((high = checksum >> 16) != 0) {
        checksum = high + (checksum & 0xffff);
    }

    return complement ? (uint16_t)~checksum : (uint16_t)checksum;
}

// 初始化随机生成器
void init_random(randam_generator_t *rg, unsigned int seed) {
    rg->seed = seed;
}

// 生成下一个随机数
unsigned int get_random(randam_generator_t *rg) {
    rg->seed = (A * rg->seed + C) % M;
    return rg->seed;
}

uint32_t random(uint32_t time_seed)
{
    uint32_t ret;
    randam_generator_t rg;
    
    init_random(&rg, time_seed);
    ret = get_random(&rg);
    return ret;
}
