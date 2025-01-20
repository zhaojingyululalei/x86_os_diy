
#include "net/netif.h"
#include "net_tools/net_threads.h"
DEFINE_PROCESS_FUNC(worker)
{
    uint8_t buf[2]={0x55,0xAA};
    int work_id = sys_getpid();
    while (1)
    {
        netif_t *netif = netif_first();
        if (!netif)
        {
            continue;
        }
        pkg_t *package = netif_get_pkg_from_inq(netif,0);
        package_write_pos(package,buf,2,0);
        netif_put_pkg_into_outq(netif,package,0);
        netif->ops->send(netif);
    }
}

void net_worker_init(void)
{
    thread_create(worker);
}