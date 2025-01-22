
#include "net/netif.h"
#include "net_tools/net_threads.h"
#include "net_msg.h"
#include "net_msg_queue.h"
/**
 * 数据包进入协议栈进行处理
 */
static int netif_in(netif_t *netif, pkg_t *package)
{
    int ret;
    netif->recv_pkg_cnt++;
    if (!netif->link_ops)
    {
        dbg_error("no link layer ops:ether? wifi?\r\n");
        return -1;
    }

    //调用链路层处理，例如ether层
    ret = netif->link_ops->in(netif,package);
    if(ret < 0)
    {
        dbg_warning("link layer ops handler a pkg fail\r\n");
        return -2;
    }

    return 0;
}
/**
 * 处理消息队列里的消息，包括数据包和应用请求
 */
static void handle(void)
{
    int ret;
    // 依次取出msgQ中的数据进行处理
    while (1)
    {
        net_msg_t *msg = net_msg_dequeue(-1);
        if (!msg)
        {
            return; // 直到取不出来为止，取不出来说明队列空了，返回
        }

        netif_t *netif = msg->netif;

        switch (msg->type)
        {
        case NET_MSG_TYPE_PKG:
            pkg_t *package = msg->package;
            net_msg_free(msg);
            ret = netif_in(netif, package);
            if (ret < 0)
            {
                netif->recv_fail_cnt++;
                package_collect(msg->package);
            }
            break;
        case NET_MSG_TYPE_REQ_FUNC:
            dbg_warning("这块功能还未实现\r\n");
            break;
        default:
            dbg_error("unkown msg type\r\n");
            break;
        }
    }
}
DEFINE_PROCESS_FUNC(worker)
{
    int ret;

    while (1)
    {
        netif_t *netif = netif_first();
        // 遍历每一块网卡,把所有数据放入msgQ中
        while (netif)
        {
            // 从网卡的inq中取package放入msgQ
            pkg_t *package = netif_get_pkg_from_inq(netif, -1);
            if (!package)
            {
                continue;
            }
            net_msg_t *pkg_msg = net_msg_create(netif, NET_MSG_TYPE_PKG, package);
            if (!pkg_msg)
            {
                package_collect(package);
                continue;
            }
            ret = net_msg_enqueue(pkg_msg, -1);
            if (ret < 0)
            {
                dbg_warning("net msgQ full,drop one pkg\r\n");
                // 内部会释放package
                package_collect(pkg_msg->package);
                net_msg_free(pkg_msg);
            }
            netif = netif_next(netif);
        }

        // 处理消息队列的数据
        handle();
    }
}

void net_worker_init(void)
{
    thread_create(worker);
}