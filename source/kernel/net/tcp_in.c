#include "tcp.h"
#include "net_tools/package.h"
#include "ipaddr.h"
#include "port.h"
static int tcp_ack_handle(tcp_t *tcp, tcp_parse_t *recv_parse)
{
    // 首先判断ACK的值对不对
    uint32_t ack_num = recv_parse->ack_num;
    // ACK的值压根不对，得做重传或者其他错误处理，暂时为空
    if ((int)(ack_num - tcp->snd.unack) < 0 || (int)(ack_num - tcp->snd.next) > 0)
    {
        ;
    }
    else
    {
        // ACK的值没问题
        // 就算发了20字节，对面只收10字节，那也是对的。那么就先确认10字节，nxt不用动
        // 如果对面迟迟收不到剩余10字节，会有重传处理，目前省略。
        tcp->snd.unack = ack_num;
    }
    return 0;
}
/**接收对面传来的数据，并且更新tcp_t，然后发送ack帧 */
static int tcp_data_handle(tcp_t *tcp, pkg_t *package, tcp_parse_t *recv_parse)
{
    bool wakeup = false;

    if (recv_parse->fin)
    {
        wakeup = true;
        tcp->rcv.next++;
    }

    if (wakeup)
    {
        if (recv_parse->fin)
        {
            sock_wait_notify(&tcp->base.conn_wait, NET_ERR_CLOSE);
            sock_wait_notify(&tcp->base.recv_wait, NET_ERR_CLOSE);
            sock_wait_notify(&tcp->base.send_wait, NET_ERR_CLOSE);
        }
        else
        {
            sock_wait_notify(&tcp->base.recv_wait, NET_ERR_CLOSE);
        }
        tcp_send_ack(tcp, recv_parse);
    }
    return 0;
}
static int tcp_time_wait(tcp_t *tcp)
{
    tcp_set_state(tcp, TCP_STATE_TIME_WAIT);
}
DEFINE_TCP_IN_HANDLE(tcp_in_handle_closed)
{
    //如果接收到reset报文，不处理
    //如果是其他类型报文，直接发送reset
    if(!parse->rst){
        tcp_send_reset(parse,host,remote);
    }
    return 0;
}
DEFINE_TCP_IN_HANDLE(tcp_in_handle_listen)
{
    return 0;
}
DEFINE_TCP_IN_HANDLE(tcp_in_handle_sys_send)
{
    /*因为都是无符号数，可能会出现循环的情况，所以处理的复杂点*/
    if ((int)(parse->ack_num - tcp->snd.init_seq) <= 0 || (int)(parse->ack_num - tcp->snd.next) > 0)
    {
        TCP_DBG_PRINT("ack err\r\n");
        tcp_send_reset(parse, host, remote);
        return -1;
    }

    // 如果对方直接返回rst报文，拒绝连接
    if (parse->rst)
    {
        if (parse->ack)
        {
            // 如果有ack标志，才是真正的发给我的rst报文
            TCP_DBG_PRINT("recv rst\r\n");
            tcp_reset(tcp, NET_ERR_RECV_RST);
        }
        // 没有ack，就是不合规范的rst报文,选择无视该报文
    }

    if (parse->syn)
    {
        tcp->rcv.init_seq = parse->seq_num;
        tcp->rcv.next = tcp->rcv.init_seq + 1;
        if (parse->ack)
        { // 想让我从哪里开始发
            tcp->snd.unack++;
        }
        // 如果收到的是syn+ack，tcp三次握手建立连接
        if (parse->ack)
        {
            tcp_send_ack(tcp, parse);
            tcp->state = TCP_STATE_ESTABLISHED;
            sock_wait_notify(&tcp->base.conn_wait, NET_ERR_OK);
        }
        else
        {
            // 如果是同时打开，即只收到了syn，4次握手
            tcp->state = TCP_STATE_SYN_RECVD;
            tcp_send_syn_ack(tcp, parse);
        }
    }

    package_collect(package);
    return 0;
}
DEFINE_TCP_IN_HANDLE(tcp_in_handle_sys_recvd)
{
    return 0;
}
DEFINE_TCP_IN_HANDLE(tcp_in_handle_established)
{
    // 在已经建立连接的情况下，又收到了syn报文
    if (parse->syn)
    {
        tcp_send_reset(parse, host, remote);
        TCP_DBG_PRINT("recv syn\r\n");
        return -1;
    }
    // 如果对方直接返回rst报文，拒绝连接
    if (parse->rst)
    {
        if (parse->ack)
        {
            // 如果有ack标志，才是真正的发给我的rst报文
            TCP_DBG_PRINT("recv rst\r\n");
            tcp_reset(tcp, NET_ERR_RECV_RST);
        }
        // 没有ack，就是不合规范的rst报文,选择无视该报文
    }

    if (parse->ack)
    {
        tcp_ack_handle(tcp, parse);
    }

    // 对方要主动关闭连接
    if (parse->fin)
    {
        tcp_set_state(tcp, TCP_STATE_CLOSE_WAIT);
    }
    tcp_data_handle(tcp, package, parse);

    package_collect(package);
    return 0;
}
DEFINE_TCP_IN_HANDLE(tcp_in_handle_fin_wait1)
{
    // 在已经建立连接的情况下，又收到了syn报文
    if (parse->syn)
    {
        tcp_send_reset(parse, host, remote);
        TCP_DBG_PRINT("recv syn\r\n");
        return -1;
    }
    // 如果对方直接返回rst报文，拒绝连接
    if (parse->rst)
    {
        if (parse->ack)
        {
            // 如果有ack标志，才是真正的发给我的rst报文
            TCP_DBG_PRINT("recv rst\r\n");
            tcp_reset(tcp, NET_ERR_RECV_RST);
        }
        // 没有ack，就是不合规范的rst报文,选择无视该报文
    }

    /*需要判断是closing状态还是Timewait状态*/
    if (parse->fin)
    {
        // 检查ack序号是否对之前发的fin帧进行了确认
        if ((int)(parse->ack_num - tcp->snd.unack) > 0)
        {
            // 如果确认了
            tcp_time_wait(tcp);
        }
        else
        {
            tcp_set_state(tcp, TCP_STATE_CLOSING);
        }
    }
    else
    {
        tcp_set_state(tcp, TCP_STATE_FIN_WAIT2);
    }
    if (parse->ack)
    {
        tcp_ack_handle(tcp, parse);
    }

    tcp_data_handle(tcp, package, parse);
    package_collect(package);
    return 0;
}
DEFINE_TCP_IN_HANDLE(tcp_in_handle_fin_wait2)
{
    // 在已经建立连接的情况下，又收到了syn报文
    if (parse->syn)
    {
        tcp_send_reset(parse, host, remote);
        TCP_DBG_PRINT("recv syn\r\n");
        return -1;
    }
    // 如果对方直接返回rst报文，拒绝连接
    if (parse->rst)
    {
        if (parse->ack)
        {
            // 如果有ack标志，才是真正的发给我的rst报文
            TCP_DBG_PRINT("recv rst\r\n");
            tcp_reset(tcp, NET_ERR_RECV_RST);
        }
        // 没有ack，就是不合规范的rst报文,选择无视该报文
    }

    if (parse->ack)
    {
        tcp_ack_handle(tcp, parse);
    }

    if (parse->fin)
    {
        tcp_time_wait(tcp);
    }

    tcp_data_handle(tcp, package, parse);
    package_collect(package);
    return 0;
}
DEFINE_TCP_IN_HANDLE(tcp_in_handle_closing)
{
    // 在已经建立连接的情况下，又收到了syn报文
    if (parse->syn)
    {
        tcp_send_reset(parse, host, remote);
        TCP_DBG_PRINT("recv syn\r\n");
        return -1;
    }
    // 如果对方直接返回rst报文，拒绝连接
    if (parse->rst)
    {
        if (parse->ack)
        {
            // 如果有ack标志，才是真正的发给我的rst报文
            TCP_DBG_PRINT("recv rst\r\n");
            tcp_reset(tcp, NET_ERR_RECV_RST);
        }
        // 没有ack，就是不合规范的rst报文,选择无视该报文
    }

    // 检查ack序号是否对之前发的fin帧进行了确认
    if ((int)(parse->ack_num - tcp->snd.unack) > 0)
    {
        tcp_time_wait(tcp);
    }
    if (parse->ack)
    {
        tcp_ack_handle(tcp, parse);
    }
    return 0;
}
DEFINE_TCP_IN_HANDLE(tcp_in_handle_time_wait)
{
    // 在已经建立连接的情况下，又收到了syn报文
    if (parse->syn)
    {
        tcp_send_reset(parse, host, remote);
        TCP_DBG_PRINT("recv syn\r\n");
        return -1;
    }
    // 如果对方直接返回rst报文，拒绝连接
    if (parse->rst)
    {
        if (parse->ack)
        {
            // 如果有ack标志，才是真正的发给我的rst报文
            TCP_DBG_PRINT("recv rst\r\n");
            tcp_reset(tcp, NET_ERR_RECV_RST);
        }
        // 没有ack，就是不合规范的rst报文,选择无视该报文
    }

    if (parse->ack)
    {
        tcp_ack_handle(tcp, parse);
    }

    if (parse->fin)
    {
        tcp_send_ack(tcp, parse);
        tcp_time_wait(tcp);
    }

    package_collect(package);
    return 0;
}
DEFINE_TCP_IN_HANDLE(tcp_in_handle_close_wait)
{
    //接收到对方的fin+ack后，我发送ack，然后进入close_wait状态
    //对方可能没收到我发的ack，对方可能会重传fin+ack
    //在对方没收到我的ack时，他就没关闭，仍然可以发其他数据
    //因此处理方法和establish一样
   tcp_in_handle_established(tcp,package,parse,remote,host);
   return 0; 
}
DEFINE_TCP_IN_HANDLE(tcp_in_handle_last_ack)
{
    // 在已经建立连接的情况下，又收到了syn报文
    if (parse->syn)
    {
        tcp_send_reset(parse, host, remote);
        TCP_DBG_PRINT("recv syn\r\n");
        return -1;
    }
    // 如果对方直接返回rst报文，拒绝连接
    if (parse->rst)
    {
        if (parse->ack)
        {
            // 如果有ack标志，才是真正的发给我的rst报文
            TCP_DBG_PRINT("recv rst\r\n");
            tcp_reset(tcp, NET_ERR_RECV_RST);
        }
        // 没有ack，就是不合规范的rst报文,选择无视该报文
    }
    /// 等到了fin的ack回应
    if (parse->ack)
    {
        tcp_reset(tcp, NET_ERR_CLOSE);
    }
    package_collect(package);
    return 0;
}

/**
 * 接受一个包，根据当前tcp的状态，对该包进行相应的处理
 */
static tcp_in_handle tcp_in_handle_tbl[] = {
    [TCP_STATE_CLOSED] = tcp_in_handle_closed,
    [TCP_STATE_LISTEN] = tcp_in_handle_listen,
    [TCP_STATE_SYN_SEND] = tcp_in_handle_sys_send,
    [TCP_STATE_SYN_RECVD] = tcp_in_handle_sys_recvd,
    [TCP_STATE_ESTABLISHED] = tcp_in_handle_established,
    [TCP_STATE_FIN_WAIT1] = tcp_in_handle_fin_wait1,
    [TCP_STATE_FIN_WAIT2] = tcp_in_handle_fin_wait2,
    [TCP_STATE_CLOSING] = tcp_in_handle_closing,
    [TCP_STATE_TIME_WAIT] = tcp_in_handle_time_wait,
    [TCP_STATE_CLOSE_WAIT] = tcp_in_handle_close_wait,
    [TCP_STATE_LAST_ACK] = tcp_in_handle_last_ack,
};

tcp_pkg_is_ok(pkg_t *package, ipaddr_t *src_ip, ipaddr_t *dest_ip, tcp_parse_t *parse)
{

    tcp_parse(package, parse);
    // tcp_show(parse);
    if (!parse->src_port || !parse->dest_port)
    {
        TCP_DBG_PRINT("port not exist\r\n");
        return -1;
    }
    if (package_checksum_peso(package, src_ip, dest_ip, IPPROTO_TCP))
    {
        TCP_DBG_PRINT("udp checksum wrong\r\n");
        return -3;
    }
    // 如果端口没开，端口不可达
    if (!net_port_is_used(parse->dest_port))
    {
        TCP_DBG_PRINT("udp prot unreachable\r\n");
        return -2;
    }
    return 0;
}
static tcp_t *tcp_find(ipaddr_t *host_ip, ipaddr_t *remote_ip, port_t host_port, port_t remote_port)
{
    list_node_t *cur = tcp_list.first;
    while (cur)
    {
        tcp_t *tcp = list_node_parent(cur, tcp_t, node);
        // tcp点对点链接，这里需要完全匹配
        if (tcp->base.target_ip.q_addr == remote_ip->q_addr &&
            tcp->base.host_ip.q_addr == host_ip->q_addr &&
            tcp->base.target_port == remote_port &&
            tcp->base.host_port == host_port)
        {

            return tcp;
        }
        cur = cur->next;
    }
    return NULL;
}
int tcp_in(pkg_t *package, ipaddr_t *remote_ip, ipaddr_t *host_ip)
{
    int ret;
    tcp_parse_t parse;
    // 检查包是否正确，顺便解析tcp包
    ret = tcp_pkg_is_ok(package, remote_ip, host_ip, &parse);
    if (ret < 0)
    {
        // 包格式不对，或者端口不对，就发送复位包
        tcp_send_reset(&parse, host_ip, remote_ip);
        TCP_DBG_PRINT("tcp pkg fomat wrong\r\n");
        return -1;
    }

    port_t remote_port = parse.src_port;
    port_t host_port = parse.dest_port;
    tcp_t *tcp = NULL;

    // 如果是成功连接后的数据交流，那么就只有一个唯一的tcp，直接用hash算法找tcp
    // 如果是发送syn后，服务器方回应的syn+ack，这时只能遍历tcp_list链表找，仍是唯一
    // 如果是服务器方的accept，收到的ack，这时如果端口重用的话，可能会有多个符合的tcp，需要用分配算法负载均衡，选择其中一个tcp
    // 成功建立连接后，要把tcp加入到hash表中

    // 先看看是否成功建立连接，如果成功建立连接就有tcp就有唯一性
    tcp = find_tcp_connection(host_ip->q_addr, host_port, remote_ip->q_addr, remote_port);
    if (tcp)
    {
        tcp_in_handle_tbl[tcp->state](tcp, package, &parse, remote_ip, host_ip);
        return 0;
    }

    // 没找到建立好的tcp链接，说明该数据帧是链接阶段的数据帧
    //  如果该包是 纯纯的请求帧（只有服务端才会收到纯纯的请求帧，因此需要考虑端口复用）
    if (parse.syn && !parse.ack)
    {
        list_node_t *cur = tcp_list.first;
        // 这里得用一些分配算法，暂时把包分给所有sockfd，应该只选定一个的
        while (cur)
        {
            tcp_t *tcpx = list_node_parent(cur, tcp_t, node);
            // 建立连接之前，bind只绑定了本地地址，remoteip port并不知道
            if ((tcpx->base.target_ip.q_addr == 0) &&
                tcpx->base.host_ip.q_addr == host_ip->q_addr &&
                (tcpx->base.target_port == 0) &&
                tcpx->base.host_port == host_port)
            {
                tcp = tcpx;
                // 到这里，就算是交给应用了，无论执行成功与否，都会由应用程序释放该包。
                pkg_t *pkg_cpy = package_clone(package);
                tcp_in_handle_tbl[tcp->state](tcp, pkg_cpy, &parse, remote_ip, host_ip);
            }
            cur = cur->next;
        }
        package_collect(package);
    }
    else
    {
        // 客户端这边，收到的包全是tcp唯一的，不过得去遍历链表找tcp，因为连接还没建立好
        tcp = tcp_find(host_ip, remote_ip, host_port, remote_port);
        if (!tcp)
        {
            tcp_send_reset(&parse, host_ip, remote_ip);
            TCP_DBG_PRINT("can not find correct tcp_t\r\n");
            return -3;
        }
        // 找到合适的tcp后，根据tcp状态调用不同的函数处理数据包
        tcp_in_handle_tbl[tcp->state](tcp, package, &parse, remote_ip, host_ip);
    }

    return 0;
    // 找合适的tcp结构处理数据包

    // 没找到合适的tcp也要发reset包
}