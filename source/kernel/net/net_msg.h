#ifndef __NET_MSG_H
#define __NET_MSG_H

#include "net_tools/package.h"

#define NET_MSG_MAX_NR 100
typedef enum _net_msg_type_t
{
    NET_MSG_TYPE_PKG,      // 消息类型是数据包
    NET_MSG_TYPE_REQ_FUNC, // 消息类型是app请求
} net_msg_type_t;

typedef struct _net_msg_t
{
    net_msg_type_t type; // 消息类型

    union
    {
        pkg_t *package;
    };

} net_msg_t;

void net_msg_init(void);
net_msg_t *net_msg_create(net_msg_type_t type, void *data);
void net_msg_free(net_msg_t *message);
#endif