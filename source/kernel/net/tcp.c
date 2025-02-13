#include "tcp.h"
#include "list.h"
#include "net_tools/net_mmpool.h"
#include "port.h"
#include "ipv4.h"
#include "algrithem.h"
#include "_time.h"
#include "rtc.h"
static uint8_t tcp_buf[TCP_BUF_MAX_NR * (sizeof(list_node_t) + sizeof(tcp_t))];
static mempool_t tcp_pool;
list_t tcp_list;

static uint8_t tcp_data_buf[TCP_BUF_MAX_NR * (sizeof(list_node_t) + sizeof(tcp_data_t))];
static mempool_t tcp_data_pool;


static int tcp_sendto(struct _sock_t *s, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest, socklen_t dest_len)
{
}
static int tcp_recvfrom(struct _sock_t *s, void *buf, size_t len, int flags,
                        struct sockaddr *src, socklen_t *addr_len)
{
}
static int tcp_setsockopt(struct _sock_t *s, int level, int optname, const char *optval, int optlen)
{
}
static int tcp_bind(struct _sock_t *s, const struct sockaddr *addr, socklen_t len)
{
}

static uint32_t tcp_alloc_iss(void)
{
    uint32_t timeseed = get_time_seed();
    return random(timeseed);
}
static int tcp_init_connect(tcp_t *tcp)
{

    tcp->snd.init_seq = tcp_alloc_iss();
    tcp->snd.next = tcp->snd.unack = tcp->snd.init_seq;

    return 0;
}
static int tcp_connect(struct _sock_t *s, const struct sockaddr *addr, socklen_t len)
{

    int ret;
    tcp_t *tcp = (tcp_t *)s;
    if (tcp->state != TCP_STATE_CLOSED)
    {
        dbg_error("tcp state err\r\n");
        return -3;
    }

    ip_route_entry_t *entry = ip_route(&s->target_ip);
    if (!entry)
    {
        dbg_warning("ip route err\r\n");
        return -6;
    }
    // 之前没绑定ip
    if (s->host_ip.q_addr == 0)
    {
        s->host_ip.q_addr = entry->netif->info.ipaddr.q_addr;
    }
    else
    {
        // 如果之前绑定了
        if (entry->netif->info.ipaddr.q_addr != s->host_ip.q_addr)
        {
            dbg_warning("the ip which bind can not ip route\r\n");
            return -4;
        }
    }
    const struct sockaddr_in *sockaddr = (const struct sockaddr_in *)addr;
    s->target_ip.q_addr = sockaddr->sin_addr.s_addr;
    s->target_port = ntohs(sockaddr->sin_port);

    // 前面可能没有绑定，因此动态分配端口
    if (s->host_port != 0)
    {
        if (net_port_is_free(s->host_port))
        {
            // 如果绑定的端口并没有开启
            dbg_warning("connect port err\r\n");
            return -5;
        }
    }
    else
    {
        s->host_port = net_port_dynamic_alloc();
    }

    // 检测tcp链接的唯一性(唯一的4元组)
    if (find_tcp_connection(tcp->base.host_ip.q_addr, tcp->base.host_port, tcp->base.target_ip.q_addr, tcp->base.target_port))
    {
        dbg_warning("tcp is already exisit\r\n");
        return -7;
    }
    // 初始化各个标志位,现在先别加hash表，都不知道是否成功连接
    // hash表里存的都是成功的链接
    ret = tcp_init_connect(tcp);
    if (ret < 0)
    {
        dbg_warning("connect fail\r\n");
        return -2;
    }

    // 发送syn数据帧
    ret = tcp_send_syn(tcp);
    if (ret < 0)
    {
        dbg_warning("send syn fail\r\n");
        return -3;
    }
    // 修改状态
    tcp_set_state(tcp, TCP_STATE_SYN_SEND);
}
static int tcp_send(struct _sock_t *s, const void *buf, size_t len, int flags)
{
}
static int tcp_recv(struct _sock_t *s, void *buf, size_t len, int flags)
{
}
static int tcp_listen(struct _sock_t *s, int backlog)
{
}
static int tcp_accept(struct _sock_t *s, struct sockaddr *addr, socklen_t *len, struct _sock_t **client)
{
}
static void tcp_destroy(struct _sock_t *s)
{
}
static int tcp_close(struct _sock_t* s){
    tcp_t* tcp = (tcp_t*)s;
    switch (tcp->state)
    {
    case TCP_STATE_CLOSED:
        tcp_free(tcp);
        break;
    //如果处于正在连接 状态
    case TCP_STATE_SYN_SEND:
    case TCP_STATE_SYN_RECVD:
        tcp_reset(tcp,NET_ERR_CLOSE);
        tcp_free(tcp);
        tcp_set_state(tcp,TCP_STATE_CLOSED);
        break;
    case TCP_STATE_ESTABLISHED:
        tcp_send_fin(tcp);
        tcp_set_state(tcp,TCP_STATE_FIN_WAIT1);
        return NET_ERR_NEED_WAIT;//调用完close的线程得等着ack
    case TCP_STATE_CLOSE_WAIT:
        tcp_send_fin(tcp);
        tcp_set_state(tcp,TCP_STATE_LAST_ACK);
        return NET_ERR_NEED_WAIT;//调用完close的线程得等着ack
    default:
        TCP_DBG_PRINT("not handle\r\n");
        return -1;
    
    }
    return 0;
}
static const sock_ops_t tcp_ops = {
    .setsockopt = tcp_setsockopt,
    .sendto = tcp_sendto,
    .recvfrom = tcp_recvfrom,
    .connect = tcp_connect,
    .send = tcp_send,
    .recv = tcp_recv,
    .bind = tcp_bind,
    .close = tcp_close,
};

static tcp_t *tcp_alloc(int tmo)
{
    tcp_t *ret = NULL;
    ret = mempool_alloc_blk(&tcp_pool, tmo);
    if (ret)
    {
        memset(ret, 0, sizeof(tcp_t));
    }
    return ret;
}
void tcp_free(tcp_t *tcp)
{
    sock_wait_destory(&tcp->base.conn_wait);
    sock_wait_destory(&tcp->base.recv_wait);
    sock_wait_destory(&tcp->base.send_wait);
    sock_wait_destory(&tcp->close_wait);
    list_remove(&tcp_list,&tcp->node);
    hash_delete_tcp_connection(tcp);
    memset(tcp,0,sizeof(tcp_t));
    mempool_free_blk(&tcp_pool, tcp);
}
/**
 * 因为啥要复位tcp协议
 */
void tcp_reset(tcp_t *tcp, net_err_t err)
{
    tcp->state = TCP_STATE_CLOSED;
    sock_wait_notify(&tcp->base.conn_wait, err);
    sock_wait_notify(&tcp->base.recv_wait, err);
    sock_wait_notify(&tcp->base.send_wait, err);
    sock_wait_notify(&tcp->close_wait,err);
    //tcp_free(tcp);去唤醒的地方 释放tcp
}
tcp_data_t *tcp_data_alloc()
{
    tcp_data_t *ret = NULL;
    ret = mempool_alloc_blk(&tcp_data_pool, -1);
    if (ret)
    {
        memset(ret, 0, sizeof(tcp_data_t));
    }
    return ret;
}
void tcp_data_free(tcp_data_t *tcp_data)
{
    mempool_free_blk(&tcp_data_pool, tcp_data);
}

sock_t *tcp_create(int family, int protocol)
{
    int ret;
    tcp_t *tcp = tcp_alloc(0);
    if (!tcp)
    {
        dbg_error(" tcp pool out of memory\r\n");
        return NULL;
    }
    int proto = protocol == 0 ? TCP_DEFAULT_PROTOCAL : protocol;
    sock_init((sock_t *)tcp, family, proto, &tcp_ops);

    // sock_wait_init(&tcp->snd.wait);
    // sock_wait_init(&tcp->rcv.wait);
    sock_wait_init(&tcp->close_wait);

    list_insert_last(&tcp_list, &tcp->node);

    return (sock_t *)tcp;
}
void tcp_init(void)
{
    list_init(&tcp_list);
    mempool_init(&tcp_pool, tcp_buf, TCP_BUF_MAX_NR, sizeof(tcp_t));
    mempool_init(&tcp_data_pool, tcp_data_buf, TCP_BUF_MAX_NR, sizeof(tcp_data_t));
}

void tcp_parse(pkg_t *package, tcp_parse_t *parse)
{
    tcp_head_t *head = package_data(package, sizeof(tcp_head_t), 0);
    parse->src_port = ntohs(head->src_port);
    parse->dest_port = ntohs(head->dest_port);
    parse->seq_num = ntohl(head->seq);
    parse->ack_num = ntohl(head->ack);
    uint16_t hlen_and_flags = ntohs(head->hlen_and_flags);
    parse->head_len = (hlen_and_flags >> 12) * 4;
    parse->urg = hlen_and_flags & TCP_URG_POS;
    parse->ack = hlen_and_flags & TCP_ACK_POS;
    parse->psh = hlen_and_flags & TCP_PSH_POS;
    parse->rst = hlen_and_flags & TCP_RST_POS;
    parse->syn = hlen_and_flags & TCP_SYN_POS;
    parse->fin = hlen_and_flags & TCP_FIN_POS;

    parse->win_size = ntohs(head->win_size);
    parse->checksum = ntohs(head->checksum);
    parse->urgptr = ntohs(head->urg_ptr);

    if (parse->head_len > sizeof(tcp_head_t))
    {
        parse->options = (uint8_t *)head + sizeof(tcp_head_t);
        parse->option_len = parse->head_len - sizeof(tcp_head_t);
    }
}
void tcp_set(pkg_t *package, const tcp_parse_t *parse)
{
    tcp_head_t *head = package_data(package, sizeof(tcp_head_t) + parse->option_len, 0);
    head->src_port = htons(parse->src_port);
    head->dest_port = htons(parse->dest_port);
    head->seq = htonl(parse->seq_num);
    head->ack = htonl(parse->ack_num);

    uint16_t hlen_and_flags = 0;
    hlen_and_flags |= ((parse->head_len / 4) << 12);
    // 设置 TCP 标志位
    if (parse->urg)
        hlen_and_flags |= (1 << 5); // 设置 URG 标志位
    if (parse->ack)
        hlen_and_flags |= (1 << 4); // 设置 ACK 标志位
    if (parse->psh)
        hlen_and_flags |= (1 << 3); // 设置 PSH 标志位
    if (parse->rst)
        hlen_and_flags |= (1 << 2); // 设置 RST 标志位
    if (parse->syn)
        hlen_and_flags |= (1 << 1); // 设置 SYN 标志位
    if (parse->fin)
        hlen_and_flags |= (1 << 0); // 设置 FIN 标志位
    head->hlen_and_flags = htons(hlen_and_flags);

    // 设置窗口大小
    head->win_size = htons(parse->win_size); // 窗口大小

    // 设置校验和和紧急指针
    head->checksum = 0;                   // 校验和
    head->urg_ptr = htons(parse->urgptr); // 紧急指针

    // 如果有 TCP 选项，填充选项字段
    if (parse->options && parse->option_len > 0)
    {
        uint8_t *option_ptr = (uint8_t *)(head + 1);           // 跳过头部字段
        memcpy(option_ptr, parse->options, parse->option_len); // 拷贝选项数据
    }
}
// TCP 报文信息打印函数
void tcp_show(const tcp_parse_t *tcp)
{
#ifdef TCP_DBG
    dbg_info("TCP Header Information:\r\n");
    dbg_info("-----------------------\r\n");
    dbg_info("Source Port: %d\r\n", tcp->src_port);
    dbg_info("Destination Port: %d\r\n", tcp->dest_port);
    dbg_info("Sequence Number: %d\r\n", tcp->seq_num);
    dbg_info("Acknowledgment Number: %d\r\n", tcp->ack_num);
    dbg_info("Header Length: %d bytes\r\n", tcp->head_len);
    dbg_info("Flags:\r\n");
    dbg_info("  URG: %s\r\n", tcp->urg ? "Yes" : "No");
    dbg_info("  ACK: %s\r\n", tcp->ack ? "Yes" : "No");
    dbg_info("  PSH: %s\r\n", tcp->psh ? "Yes" : "No");
    dbg_info("  RST: %s\r\n", tcp->rst ? "Yes" : "No");
    dbg_info("  SYN: %s\r\n", tcp->syn ? "Yes" : "No");
    dbg_info("  FIN: %s\r\n", tcp->fin ? "Yes" : "No");
    dbg_info("Window Size: %d\r\n", tcp->win_size);
    dbg_info("Checksum: 0x%04x\r\n", tcp->checksum);
    dbg_info("Urgent Pointer: %d\r\n", tcp->urgptr);

    // 打印选项（如果有）
    if (tcp->options && tcp->option_len > 0)
    {
        dbg_info("Options (%d bytes):\r\n", tcp->option_len);
        for (uint16_t i = 0; i < tcp->option_len; i++)
        {
            dbg_info("  0x%02x ", tcp->options[i]);
            if ((i + 1) % 8 == 0)
                dbg_info("\r\n"); // 每 8 个字节换行
        }
        dbg_info("\r\n");
    }
    else
    {
        dbg_info("Options: None\r\n");
    }
    dbg_info("-----------------------\r\n");
#endif
}