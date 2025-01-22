#include "net/net_syscall.h"
#include "debug.h"

void rtl8139_drive_test(void)
{
    const char* if_name = "eth0";
    sys_netif_create(if_name,NETIF_TYPE_ETH);
    netif_t* netif = find_netif_by_name(if_name);
    netif_set_ip(netif,"192.168.169.50");
    netif_set_gateway(netif,"192.168.169.1");
    netif_set_mask(netif,"255.255.255.0");
    sys_netif_open(if_name);
    sys_netif_active(if_name);


    sys_netif_show();
}
#include "net/ipaddr.h"
void ipaddr_test(void)
{
    // const char* ip_str = "192.168.169.50";
    // ipaddr_t ip;
    // ipaddr_s2n(ip_str,&ip);

    // char ip_buf[20]={0};
    // ipaddr_n2s(&ip,ip_buf,20);
    
    // dbg_info("ip buf:%s\r\n",ip_buf);

    // const char* mac_str = "52:54:00:c9:18:27";
    // hwaddr_t mac;
    // char mac_buf[20]={0};

    // mac_s2n(mac_str,&mac);
    // mac_n2s(&mac,mac_buf);
    // dbg_info("mac buf:%s\r\n",mac_buf);

    hwaddr_t *hw = get_mac_broadcast();

}
void net_test(void)
{
    rtl8139_drive_test();
    
}