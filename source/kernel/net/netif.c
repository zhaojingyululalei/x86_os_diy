#include "netif.h"
#include "net_tools/net_mmpool.h"
#include "net_tools/package.h"
#include "net_tools/msgQ.h"
static mempool_t netif_pool;
static uint8_t netif_buf[NETIF_MAX_NR * (sizeof(netif_t) + sizeof(list_node_t))];
static list_t netif_list;
static lock_t netif_list_locker;

void netif_init(void)
{

    mempool_init(&netif_pool, netif_buf, NETIF_MAX_NR, sizeof(netif_t));
    list_init(&netif_list);
    lock_init(&netif_list_locker);
}

void netif_destory(void)
{
    mempool_destroy(&netif_pool);
    lock_destory(&netif_list_locker);
    list_destory(&netif_list);
}

netif_t *netif_alloc(void)
{
    return (netif_t *)mempool_alloc_blk(&netif_pool, -1);
}
int netif_free(netif_t *netif)
{
    int ret;
    while(!msgQ_is_empty(&netif->in_q))
    {
        pkg_t* package = msgQ_dequeue(&netif->in_q,-1);
        package_collect(package);
    }
    while(!msgQ_is_empty(&netif->out_q))
    {
        pkg_t* package = msgQ_dequeue(&netif->out_q,-1);
        package_collect(package);
    }
    msgQ_destory(&netif->in_q);
    msgQ_destory(&netif->out_q);
    memset(netif, 0, sizeof(netif_t));
    ret = mempool_free_blk(&netif_pool, netif);
    if (ret < 0)
    {
        dbg_error("netif free fail\r\n");
        return ret;
    }
    return 0;
}
int netif_insert(netif_t *netif)
{
    lock(&netif_list_locker);
    list_insert_last(&netif_list, &netif->node);
    unlock(&netif_list_locker);
    return 0;
}

netif_t *netif_remove(netif_t *netif)
{
    lock(&netif_list_locker);
    list_t *list = &netif_list;
    list_node_t *cur = list->first;
    while (cur)
    {
        if (cur == &netif->node)
        {
            list_remove(list, cur);
            unlock(&netif_list_locker);
            return netif;
        }
        cur = cur->next;
    }
    unlock(&netif_list_locker);
    return NULL;
}
netif_t *netif_first()
{
    list_t *list = &netif_list;
    list_node_t *fnode = list->first;
    netif_t *ret = list_node_parent(fnode, netif_t, node);
    return ret;
}
netif_t *netif_next(netif_t *netif)
{
    if (!netif)
    {
        dbg_error("netif is NULL,not have next\r\n");
        return NULL;
    }
    list_node_t *next = netif->node.next;
    if (!next)
    {
        return NULL;
    }
    else
    {
        return list_node_parent(next, netif_t, node);
    }
}

pkg_t *netif_get_pkg_from_outq(netif_t *netif, int timeout)
{
    pkg_t *package = (pkg_t *)msgQ_dequeue(&netif->out_q, timeout);
    if (!package)
    {
        return NULL;
    }
    package_lseek(package, 0);
    return package;
}

pkg_t *netif_get_pkg_from_inq(netif_t *netif, int timeout)
{
    pkg_t *package = (pkg_t *)msgQ_dequeue(&netif->in_q, timeout);
    if (!package)
    {
        return NULL;
    }
    package_lseek(package, 0);
    return package;
}

int netif_put_pkg_into_outq(netif_t *netif, pkg_t *package, int timeout)
{
    package_lseek(package, 0);
    return msgQ_enqueue(&netif->out_q, (void *)package, timeout);
}

int netif_put_pkg_into_inq(netif_t *netif, pkg_t *package, int timeout)
{
    package_lseek(package, 0);
    return msgQ_enqueue(&netif->in_q, (void *)package, timeout);
}

netif_t *find_netif_by_name(const char *if_name)
{
    /*遍历网络接口链表，找名字为if_name的接口*/
    int name_len = strlen(if_name);
    netif_t *cur_if = netif_first();
    netif_t *target_if = NULL;
    while (cur_if)
    {
        if (strncmp(if_name, cur_if->info.name, name_len) == 0)
        {
            target_if = cur_if;
        }
        cur_if = netif_next(cur_if);
    }
    if (!target_if)
    {
        dbg_error("there is no netif name:%s\r\n", if_name);
        return -1;
    }
    return target_if;
}
