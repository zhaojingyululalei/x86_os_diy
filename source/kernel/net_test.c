#include "net/net_syscall.h"
#include "debug.h"
#include "net/arp.h"
#include "net/net_tools/soft_timer.h"
#include "net/ipv4.h"
#include "net/protocal.h"
DEFINE_TIMER_FUNC(timer_0_handle)
{
    int *x = (int *)arg;
    int *ret = malloc(sizeof(int));
    *ret = *x + 10;
    dbg_info("in timer handler .. arg:%d  ret:%d\r\n", *x, *ret);
    return ret;
}
DEFINE_TIMER_FUNC(timer_1_handle)
{
    int *x = (int *)arg;
    int *ret = malloc(sizeof(int));
    *ret = *x + 10;
    dbg_info("in timer handler .. arg:%d  ret:%d\r\n", *x, *ret);
    return ret;
}
DEFINE_TIMER_FUNC(timer_2_handle)
{
    int *x = (int *)arg;
    int *ret = malloc(sizeof(int));
    *ret = *x + 10;
    dbg_info("in timer handler .. arg:%d  ret:%d\r\n", *x, *ret);
    return ret;
}
DEFINE_TIMER_FUNC(timer_3_handle)
{
    int *x = (int *)arg;
    int *ret = malloc(sizeof(int));
    *ret = *x + 10;
    dbg_info("in timer handler .. arg:%d  ret:%d\r\n", *x, *ret);
    return ret;
}
soft_timer_t timer_0, timer_1, timer_2, timer_3;
int a = 0, b = 1, c = 2, d = 3;
int *ret0 = NULL;
int *ret1 = NULL;
int *ret2 = NULL;
int *ret3 = NULL;
void timer_test(void)
{
    int count = 100;

    soft_timer_add(&timer_0, SOFT_TIMER_TYPE_ONCE, 100, "timer_0", timer_0_handle, &a, &ret0);
    soft_timer_add(&timer_2, SOFT_TIMER_TYPE_PERIOD, 8000, "timer_2", timer_2_handle, &c, &ret2);
    soft_timer_add(&timer_1, SOFT_TIMER_TYPE_PERIOD, 1000, "timer_1", timer_1_handle, &b, &ret1);
    soft_timer_add(&timer_3, SOFT_TIMER_TYPE_PERIOD, 2000, "timer_3", timer_3_handle, &d, &ret3);
    soft_timer_list_print();

    while (1)
    {
        sys_sleep_ms(100);
        soft_timer_scan_list(100);
        // count--;
    }
    dbg_info("*ret0 =%d\r\n", ret0 ? *ret0 : -1);
    dbg_info("*ret1 =%d\r\n", ret1 ? *ret1 : -1);
    dbg_info("*ret2 =%d\r\n", ret2 ? *ret2 : -1);
    dbg_info("*ret3 =%d\r\n", ret3 ? *ret3 : -1);
}
void arp_test(netif_t *netif)
{
    ipaddr_t ip;
    ipaddr_s2n("192.168.169.40", &ip);
    arp_send_request(netif, &ip);
}

void rtl8139_drive_test(void)
{
    int ret;
    const char *if_name = "eth0";
    sys_netif_create(if_name, NETIF_TYPE_ETH);
    netif_t *netif = find_netif_by_name(if_name);
    netif_set_ip(netif, "192.168.169.50");
    netif_set_gateway(netif, "192.168.169.1");
    netif_set_mask(netif, "255.255.255.0");
    sys_netif_open(if_name);
    sys_netif_active(if_name);

    sys_netif_show();

    ipaddr_t ip;
    ipaddr_s2n("192.168.169.40", &ip);
    char data_buf[2] = {0x55, 0xaa};
    pkg_t *pkg = package_create(data_buf, 2);
    ret = ipv4_out(pkg,PROTOCAL_TYPE_ICMPV4,&ip);
    if(ret < 0)
    {
        package_collect(pkg);
    }
    
    
}
#include "net/ipaddr.h"
void ipaddr_test(void)
{

    uint16_t x = 0;
    uint8_t y = 1;
    x += y<<8;
    dbg_info("x = %d\r\n",x);
}
void net_test(void)
{
    rtl8139_drive_test();
    //ipaddr_test();
    // timer_test();
}