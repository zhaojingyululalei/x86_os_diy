#include "udp.h"
#include "net_tools/net_mmpool.h"
#include "debug.h"
#include "ipv4.h"
#include "port.h"
static uint8_t udp_buf[UDP_BUF_MAX_NR * (sizeof(list_node_t) + sizeof(udp_t))];
static mempool_t udp_pool;
static list_t udp_list;

static uint8_t udp_data_buf[UDP_BUF_MAX_NR * (sizeof(list_node_t) + sizeof(udp_data_t))];
static mempool_t udp_data_pool;


static udp_data_t* udp_data_alloc(void)
{
    udp_data_t* ret = mempool_alloc_blk(&udp_data_pool,-1);
    if(!ret)
        return NULL;
    memset(ret,0,sizeof(udp_data_t));
    return ret;
}
static void udp_data_free(udp_data_t* data)
{
    package_collect(data->package);
    mempool_free_blk(&udp_data_pool,data);
}
static int udp_recvfrom(struct _sock_t *s, void *buf, size_t len, int flags,
                        struct sockaddr *src, socklen_t *addr_len)
{
    int ret;

    udp_t *udp = (udp_t *)s;

    list_node_t *udp_data_node = list_remove_first(&udp->recv_list);
    udp_data_t* udp_data = list_node_parent(udp_data_node, udp_data_t, node);

    pkg_t* package = udp_data->package;

    int cpy_len = len > package->total ? package->total : len;
    ret = package_read_pos(package, buf, cpy_len, 0);
    if (ret < 0)
    {
        list_insert_first(&udp->recv_list, &udp_data->node);
        dbg_error("package_read fault\r\n");
        return -2;
    }
    //成功读取数据到buf
    struct sockaddr_in src_addr;
    src_addr.sin_addr.s_addr = udp_data->from_ip.q_addr;
    src_addr.sin_port = udp_data->from_port;
    src_addr.sin_family = s->family;

    memcpy(src, &src_addr, sizeof(struct sockaddr_in));
    *addr_len = sizeof(struct sockaddr_in);
    package_collect(package);
    return cpy_len;
}
static int udp_sendto(struct _sock_t *s, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest, socklen_t dest_len)
{
    int ret;
    pkg_t *package = package_create(buf, len);
    struct sockaddr_in *addr = (struct sockaddr_in *)dest;
    if (addr->sin_family != s->family)
    {
        dbg_error("sockaddr set wrong :in filed sin_family\r\n");
        return -1;
    }
    // 之前没绑定端口，动态分配一个
    if (s->host_port == 0)
    {
        port_t host_port = net_port_dynamic_alloc();
        s->host_port = host_port;
    }
    return udp_out(s->host_port, addr->sin_port, &addr->sin_addr, package);

    // ret = ipv4_out(package, s->protocal, &addr->sin_addr);
    // if (ret < 0)
    // {
    //     return -1;
    // }
    // else
    // {
    //     return ret;
    // }
}
static int udp_setsockopt(struct _sock_t *s, int level, int optname, const char *optval, int optlen)
{
    switch (level)
    {
    case SOL_SOCKET:

        if (optname & SO_RCVTIMEO)
        {
            struct timeval *time = (struct timeval *)optval;
            int tmo = time->tv_sec * 1000 + time->tv_usec / 1000;
            sock_wait_set(s, tmo, SOCK_RECV_WAIT);
        }
        if (optname & SO_SNDTIMEO)
        {
            struct timeval *time = (struct timeval *)optval;
            int tmo = time->tv_sec * 1000 + time->tv_usec / 1000;
            sock_wait_set(s, tmo, SOCK_SEND_WAIT);
        }

    default:
        dbg_error("unkown setsockopt optname\r\n");
        return -1;
    }
    return 0;
}
static const sock_ops_t udp_ops = {
    .setsockopt = udp_setsockopt,
    .sendto = udp_sendto,
    .recvfrom = udp_recvfrom,
};
sock_t *udp_create(int family, int protocol)
{
    int ret;
    udp_t *udp = mempool_alloc_blk(&udp_pool, -1);
    if (!udp)
    {
        dbg_error(" udp pool out of memory\r\n");
        return NULL;
    }
    memset(udp, 0, sizeof(udp_t));
    int proto = protocol == 0 ? UDP_DEFAULT_PROTOCAL : protocol;
    sock_init((sock_t *)udp, family, proto, &udp_ops);
    list_insert_last(&udp_list, &udp->node);
    return (sock_t *)udp;
}

int udp_out(port_t src_port, port_t dest_port, ipaddr_t *dest_ip, pkg_t *package)
{
    package_add_headspace(package, sizeof(udp_head_t));
    udp_head_t *udp_head = package_data(package, sizeof(udp_head_t), 0);
    udp_head->src_port = htons(src_port);
    udp_head->total_len = htons(package->total);
    udp_head->dest_port = dest_port; // 之前转过了
    udp_head->checksum = 0;
    ip_route_entry_t *entry = ip_route(dest_ip);
    if (!entry)
    {
        dbg_warning("ip route fail\r\n");
        return -4;
    }
    ipaddr_t* src_ip = &entry->netif->info.ipaddr;

    udp_head->checksum = package_checksum_peso(package,src_ip,dest_ip,IPPROTO_UDP ) ;
    return ipv4_out(package, PROTOCAL_TYPE_UDP, dest_ip);
}

static void udp_parse(udp_head_t* head,udp_parse_t* parse)
{
    parse->src_port = ntohs(head->src_port);
    parse->dest_port = ntohs(head->dest_port);
    parse->total_len = ntohs(head->total_len);
    parse->checksum = head->checksum;
}
static int udp_is_ok(pkg_t* package,ipaddr_t* src_ip,ipaddr_t* dest_ip,udp_parse_t* parse)
{
    if(package_checksum_peso(package,src_ip,dest_ip,IPPROTO_UDP))
    {
        dbg_warning("udp checksum wrong\r\n");
        return -1;
    }
    //如果端口没开，端口不可达
    if(!net_port_is_used(parse->dest_port))
    {
        dbg_warning("udp prot unreachable\r\n");
        return -3;
    }
    return 0;
}

int udp_in(pkg_t* package,ipaddr_t* src_ip,ipaddr_t* dest_ip)
{
    int ret;
    udp_head_t* udp_head = package_data(package,sizeof(udp_head_t),0);
    udp_parse_t udp_parsed;
    udp_parse(udp_head,&udp_parsed);

    ret = udp_is_ok(package,src_ip,dest_ip,&udp_parsed);
    if(ret<0){
        return ret;
    }
    //去udp头
    package_shrank_front(package,sizeof(udp_head_t));

    port_t host_port = udp_parsed.dest_port;
    ipaddr_t *host_ip = dest_ip;
    //寻找 把数据挂到相应的udp recvlist中
    udp_t* udp =NULL;
    list_node_t* cur_node = udp_list.first;
    while (cur_node)
    {
        udp_t* cur_udp = list_node_parent(cur_node,udp_t,node);
        if(cur_udp->base.host_port == host_port)
        {
            udp = cur_udp;
            //ip是0，表示接受所有网卡的数据
            if(cur_udp->base.host_ip.q_addr == host_ip->q_addr || cur_udp->base.host_ip.q_addr == 0)
            {
                udp_data_t* udp_data = udp_data_alloc();
                udp_data->from_ip.q_addr = src_ip->q_addr;
                udp_data->from_port = udp_parsed.src_port;
                udp_data->package = package;
                list_insert_last(&cur_udp->recv_list,&udp_data->node);
                sock_wait_notify(&cur_udp->base.recv_wait);
            }
        }
        cur_node = cur_node->next;
    }
    //一个合适的socket都没找到
    if(!udp)
    {
        return -1;
    }
    return 0;
}
void udp_init(void)
{
    list_init(&udp_list);
    mempool_init(&udp_pool, udp_buf, UDP_BUF_MAX_NR, sizeof(udp_t));
    mempool_init(&udp_data_pool,udp_data_buf,UDP_BUF_MAX_NR,sizeof(udp_data_t));
}
