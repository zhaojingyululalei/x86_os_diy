#include "net_msg_queue.h"
#include "net_msg.h"
#include "net_tools/package.h"
#include "net_worker.h"
#include "netif.h"
void net_init(void)
{
    //数据包分配结构
    package_pool_init();
    //核心的消息队列 以及消息分配结构
    net_mq_init();
    net_msg_init();
    netif_init();
    net_worker_init();


}