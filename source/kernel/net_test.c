#include "net/net_syscall.h"
void net_test(void)
{
    const char* if_name = "eth0";
    sys_netif_create(if_name,NETIF_TYPE_ETH);
    sys_netif_open(if_name);
    sys_netif_active(if_name);

    netif_t* netif = find_netif_by_name(if_name);
    
}