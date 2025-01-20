#include "net_tools/net_mmpool.h"
#include "net_msg.h"
#include "list.h"
#include "net_tools/msgQ.h"
#include "string.h"
static mempool_t net_msg_pool;
static uint8_t net_msg_buf[(sizeof(net_msg_t)+sizeof(list_node_t))*NET_MSG_MAX_NR];

void net_msg_init(void)
{
    mempool_init(&net_msg_pool,net_msg_buf,NET_MSG_MAX_NR,sizeof(net_msg_t));

}

net_msg_t* net_msg_create(net_msg_type_t type,void* data)
{
    if(!data)
    {
        return NULL;
    }

    switch (type)
    {
    case NET_MSG_TYPE_PKG:
        pkg_t* pkg = (pkg_t*)data;
        if(pkg->total <= 0)
        {
            return NULL;//空包不往队列里面放
        }
        net_msg_t *message = (net_msg_t*)mempool_alloc_blk(&net_msg_pool,-1);
        if(!message)
        {
            //暂时没有空闲消息块可供使用
            return NULL;
        }
        else
        {
            message->type = type;
            message->package = pkg;
            return message;
        }
        break;
    case NET_MSG_TYPE_REQ_FUNC:
        break;
    
    default:
        dbg_error("unkown net msg type\r\n");
        return NULL;
        break;
    }
    return NULL;
}

void net_msg_free(net_msg_t* message)
{
    memset(message,0,sizeof(net_msg_t));
    mempool_free_blk(&net_msg_pool,message);
}

