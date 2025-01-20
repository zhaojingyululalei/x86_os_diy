#ifndef __IPADDR_H
#define __IPADDR_H
#include "types.h"
#include "net_cfg.h"
/**
 * @brief IP地址
 */
typedef struct _ipaddr_t
{
    enum
    {
        IPADDR_V4,
    } type; // 地址类型

    union
    {
        uint32_t q_addr;                 // 32位整体描述
        uint8_t a_addr[IPADDR_ARRY_LEN]; // 数组描述
    };
} ipaddr_t;

typedef struct _hwaddr_t
{

    uint8_t addr[MACADDR_ARRAY_LEN];

} hwaddr_t;

#endif