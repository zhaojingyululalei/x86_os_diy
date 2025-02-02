#include "net/net_syscall.h"
#include "debug.h"
#include "net/arp.h"
#include "net/net_tools/soft_timer.h"
#include "net/ipv4.h"
#include "net/net_submit.h"
#include "net/protocal.h"
#include "net/socket.h"
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

int net_task_test(uint16_t x)
{
    return x + 10;
}

#include "net/net_syscall.h"
#include "net/icmpv4.h"
/**目前有个字段没加，时间戳字段没有加
 * 能保证正确收发
 */
pkg_t *create_icmp_req_pkg(char *data_buf, int data_len)
{
    pkg_t *package = package_create(data_buf, data_len);
    package_add_headspace(package, sizeof(icmpv4_t));
    icmpv4_t *icmp = package_data(package, sizeof(icmpv4_t), 0);
    icmp->head.code = 0;
    icmp->head.type = 8;
    icmp->head.checksum = 0;
    icmp->Identifier = 1;
    icmp->Sequence = 1;

    icmp->head.checksum = package_checksum16(package, 0, package->total, 0, 1);
    return package;
}
void ping_test(void)
{
    int len;

    uint8_t data_buf[1000] = {0};
    uint8_t buf[2000] = {0};
    uint8_t recv_buf[2000] = {0};

    memset(data_buf, 0xAA, 1000);
    int sockfd = sys_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    struct sockaddr_in addr;
    struct timeval time;
    time.tv_sec = 1;
    time.tv_usec = 0;
    inet_pton(AF_INET, "192.168.169.40", &addr.sin_addr);
    pkg_t *package = create_icmp_req_pkg(data_buf, 1000);
    package_read_pos(package, buf, package->total, 0);
    len = sys_sendto(sockfd, buf, package->total, 0, &addr, sizeof(struct sockaddr_in));
    int ret = sys_setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(struct timeval));
    len = sys_recvfrom(sockfd, recv_buf, 2000, 0, &addr, sizeof(struct sockaddr_in));
    pkg_t *recv_pkg = package_create(recv_buf, len);
    icmpv4_t *icmp = package_data(recv_pkg, sizeof(icmpv4_t), 20); // 跳过ip头
    return;
}
#pragma pack(1)
typedef struct _icmp_req_t
{
    icmpv4_t icmp;
    time_t time_stamp;
    uint8_t data_buf[2000];
} icmp_req_t;
#pragma pack()
/*ping 192.168.169.40 -c1 -s100*/
#include "rtc.h"
#include "algrithem.h"
void ping_run(const char *str_ip, int c, int s)
{
    bool flag = false;
    uint8_t recv_buf[2000] = {0};
    tm_t clock_time;
    for (int i = 0; i < c; i++)
    {
        // 构造icmp请求包
        int len;
        icmp_req_t icmp_pkg;
        memset(icmp_pkg.data_buf, 0xAA, s);
        icmp_pkg.icmp.head.code = 0;
        icmp_pkg.icmp.head.type = 8;
        icmp_pkg.icmp.head.checksum = 0;
        icmp_pkg.icmp.Identifier = 1;
        icmp_pkg.icmp.Sequence = i + 1;
        sys_get_clocktime(&clock_time);
        icmp_pkg.time_stamp = sys_mktime(&clock_time);
        int check_len = sizeof(icmp_req_t) - (2000 - s);
        icmp_pkg.icmp.head.checksum = checksum16(0, &icmp_pkg, check_len, 0, 1);

        int sockfd = sys_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

        struct timeval time;
        time.tv_sec = 3;
        time.tv_usec = 0;
        //sys_setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(struct timeval));

        struct sockaddr_in addr;
        inet_pton(AF_INET, "192.168.169.40", &addr.sin_addr);
        len = sys_sendto(sockfd, &icmp_pkg, check_len, 0, &addr, sizeof(struct sockaddr_in));

        len = sys_recvfrom(sockfd, recv_buf, 2000, 0, &addr, sizeof(struct sockaddr_in));
        if (len <= 0)
        {
            continue;
        }
        int icmp_req_head_len = sizeof(ipv4_header_t) + sizeof(icmpv4_t) + sizeof(time_t);
        uint8_t *start = recv_buf + icmp_req_head_len;
        int j;
        for (j = 0; j < s; j++, start++)
        {
            if (icmp_pkg.data_buf[j] == *start)
            {
                continue;
            }
            else
            {
                break;
            }
        }
        if (j != s)
        {
            continue; // 如果数据部分有不一样的，接受数据失败了，后面就不用输出信息了
        }
        flag = true;
        ipv4_header_t *ipv4_head = recv_buf;
        uint8_t ttl = ipv4_head->ttl;
        icmp_req_t *icmp_reply = recv_buf + sizeof(ipv4_header_t); // 略过ipv4头
        time_t old_time = icmp_reply->time_stamp;
        memset(&clock_time, 0, sizeof(tm_t));
        sys_get_clocktime(&clock_time);
        time_t new_time = sys_mktime(&clock_time);
        int diff_ms = new_time - old_time;

        dbg_info("%dbytes recv from %s:seq=%d,ttl=%d,time=%dms\r\n", len, str_ip, i + 1, ttl, diff_ms);
        sys_sleep_ms(500);
    }

    if (!flag)
    {
        // 没有一次信息输出
        dbg_info("ping time out\r\n");
    }
}
sem_t sem_ello;
task_t hello_test;
void* hellofunc(void* arg)
{
    sys_sleep_ms(2000);
    sys_sem_notify(&sem_ello);
    while (1)
    {
        dbg_info("hello\r\n");
        sys_sleep_ms(2000);
        sys_sem_notify(&sem_ello);
    }
    
    
}
void net_socket_test(void)
{
    sys_sem_init(&sem_ello,0);
    ping_run("192.168.169.40", 2, 500);
    create_kernel_process(&hello_test,hellofunc);
    sys_sem_wait(&sem_ello);
    dbg_info("wake up\r\n");
    sys_sem_wait(&sem_ello);
    dbg_info("wake up agin\r\n");
    //ping_run("192.168.169.40", 1, 500);
    return;
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
    net_socket_test();

    return;
}
#include "net/ipaddr.h"
void ipaddr_test(void)
{

    uint16_t x = 0;
    uint8_t y = 1;
    x += y << 8;
    dbg_info("x = %d\r\n", x);
}
void net_test(void)
{
    rtl8139_drive_test();
    // ipaddr_test();
    //  timer_test();
}