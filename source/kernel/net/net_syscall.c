#include "net_syscall.h"
#include "dev/chr/rtl8139.h"

static rtl8139_priv_t priv;
/**
 * 创建netif结构
 */
int sys_netif_create(const char *if_name, netif_type_t type)
{
    int ret;
    netif_t *target;
    target = netif_alloc();
    if (target)
    {
        target->info.type = type;
        target->state = NETIF_STATE_CLOSE;
        strcpy(target->info.name, if_name);
        target->mtu = NETIF_MTU_DEFAULT;

        // 装驱动 并初始化相应硬件(调用open初始化)
        switch (type)
        {
        case NETIF_TYPE_ETH:
            target->ops = &netdev8139_ops; // 安装
            target->ex_data = &priv;
            
            break;

        default:
            dbg_error("Unrecognized network card type\r\n");
            goto create_fail;
            break;
        }

        /*加入netif队列*/
        netif_insert(target);
    }
    else
    {
        dbg_error("The number of network cards has reached the maximum limit,can not create one more\r\n");
        goto create_fail;
    }
    return 0;
create_fail:
    netif_free(target);
    return -1;
}
/**
 * 初始化netif拥有的资源
 * 初始化netif消息队列
 */
int sys_netif_open(const char *if_name)
{
    int ret;
    /*遍历网络接口链表，找名字为if_name的接口*/
    netif_t *target_if = NULL;
    target_if = find_netif_by_name(if_name);
    if (!target_if)
    {
        dbg_error("netif_open fail:there is no netif name:%s\r\n", if_name);
        return -1;
    }
    else
    {
        // 找到了接口  打开接口
        target_if->state = NETIF_STATE_OPEN;
        // 初始化netif接口的所有资源
        msgQ_init(&target_if->in_q, target_if->in_q_buf, NETIF_PKG_CACHE_CAPACITY);
        msgQ_init(&target_if->out_q, target_if->out_q_buf, NETIF_PKG_CACHE_CAPACITY);
        
    }
    return 0;
}
/**
 * 激活网卡
 * 网卡中断开始工作
 */
int sys_netif_active(const char *if_name)
{
    int ret;
    /*遍历网络接口链表，找名字为if_name的接口*/
    netif_t *target_if = NULL;
    target_if = find_netif_by_name(if_name);
    if (target_if)
    {
        if(target_if->state != NETIF_STATE_OPEN)
        {
            dbg_error("8139 is not in the open state and cannot be activated\r\n");
            return -3;
        }
        ret = target_if->ops->open(target_if, &priv); // 初始化8139网卡，中断开始工作
        if (ret < 0)
        {
            dbg_error("8139 failed to function properly, initialization failed\r\n");
            return -1;
        }

        target_if->state = NETIF_STATE_ACTIVE;

    }
    else
    {
        return -2;
    }
    return 0;
}


int sys_netif_deactive(const char *if_name)
{
    int ret;
    /*遍历网络接口链表，找名字为if_name的接口*/
    netif_t *target_if = NULL;
    target_if = find_netif_by_name(if_name);
    if (target_if)
    {
        if(target_if->state != NETIF_STATE_ACTIVE)
        {
            dbg_error("8139 is not in the active state and cannot be die\r\n");
            return -3;
        }
        ret = target_if->ops->close(target_if); // 初始化8139网卡，中断开始工作
        if (ret < 0)
        {
            dbg_error("8139 failed to function properly, initialization failed\r\n");
            return -1;
        }

        target_if->state = NETIF_STATE_DIE;

    }
    else
    {
        return -2;
    }
    return 0;
}

int sys_netif_close(const char *if_name)
{
    int ret;
    /*遍历网络接口链表，找名字为if_name的接口*/
    netif_t *target_if = NULL;
    target_if = find_netif_by_name(if_name);
    if (target_if)
    {
        if(target_if->state != NETIF_STATE_DIE)
        {
            dbg_error("8139 is still working,not in the die state and cannot be close\r\n");
            return -3;
        }
        
        target_if->state = NETIF_STATE_CLOSE;
        //释放网卡资源
        //队列中所有的数据包取出来并释放掉
        netif_remove(target_if);
        netif_free(target_if);

    }
    else
    {
        return -2;
    }
    return 0;
}



