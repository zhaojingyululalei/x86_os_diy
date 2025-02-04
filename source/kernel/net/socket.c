#include "socket.h"
#include "net_submit.h"
#include "socket_param.h"
#include "sock.h"
/**
 * 主要负责提交任务给工作线程
 * 而且都是系统调用
 * */

int sys_socket(int family, int type, int protocal)
{
    sock_create_param_t param;
    param.family = family;
    param.type = type;
    param.protocal = protocal;

    int fd = net_task_submit(sock_create, &param);
    return fd;
}

int sys_connect(int sockfd, const struct sockaddr *addr, socklen_t len)
{
    sock_connect_param_t param;
    param.sockfd = sockfd;
    param.addr = addr;
    param.len = len;
    int ret;
    sock_t *sock = socket_from_index(sockfd)->sock;
    // 只有tcp协议的connect才需要等待，raw和udp的connect不需要等待
    if (sock->protocal == IPPROTO_TCP)
    {
        ret = sock_wait_enter(&sock->conn_wait);
        if (ret < 0)
        {
            return -1;
        }
    }

    ret = net_task_submit(sock_connect, &param);
    return ret;
}

int sys_sendto(int sockfd, const void *buf, size_t buf_len, int flags,
               const struct sockaddr *dest, socklen_t dest_len)
{
    sock_sendto_param_t param;
    param.sockfd = sockfd;
    param.addr = dest;
    param.addr_len = dest_len;

    int remain_len = buf_len;
    int ret_len = 0, send_len = 0;
    while (remain_len > 0)
    {
        param.buf = buf + send_len;
        param.buf_len = remain_len;

        ret_len = net_task_submit(sock_sendto, &param);
        if (ret_len < 0)
        {
            return -3;
        }
        else
        {
            remain_len -= ret_len;
            send_len += ret_len;
        }
    }

    return send_len;
}
int sys_send(int sockfd, const void *buf, size_t len, int flags)
{
    sock_send_param_t param;
    param.sockfd = sockfd;
    param.buf = buf;
    param.len = len;
    param.flags = flags;

    int remain_len = len;
    int ret_len = 0, send_len = 0;
    while (remain_len > 0)
    {
        param.buf = buf + send_len;
        param.len = remain_len;

        ret_len = net_task_submit(sock_send, &param);
        if (ret_len < 0)
        {
            return -3;
        }
        else
        {
            remain_len -= ret_len;
            send_len += ret_len;
        }
    }

    return send_len;
}
int sys_recv(int sockfd, void *buf, size_t len, int flags)
{
    sock_recv_param_t param;
    param.sockfd = sockfd;
    param.buf = buf;
    param.len = len;
    param.flags = flags;

    int ret;
    int ret_len = 0, recv_len = 0;
    sock_t *sock = socket_from_index(sockfd)->sock;

    ret = sock_wait_enter(&sock->recv_wait);
    if (ret < 0)
    {
        return recv_len;
    }

    ret_len = net_task_submit(sock_recv, &param);
    if (ret_len < 0)
    {

        return -1;
    }
    recv_len += ret_len;

    return recv_len;
}
/**
 * 默认阻塞等，可以setsockopt设置阻塞时长
 * 多个线程调用同一个recvfrom，使用同一个sockfd，接收到的数据是不同的数据包
 * 有数据到达，立马接收:如果打算接受2000字节，但是只有1000字节的数据包，就返回1000，不会一直等到2000字节才返回
 */
int sys_recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src, socklen_t *addr_len)
{

    sock_recvfrom_param_t param;
    param.sockfd = sockfd;
    param.buf = buf;
    param.buf_len = len;
    param.flags = flags;
    param.addr = src;
    param.addr_len = addr_len;
    int ret;
    int ret_len = 0, recv_len = 0;
    sock_t *sock = socket_from_index(sockfd)->sock;

    ret = sock_wait_enter(&sock->recv_wait);
    if (ret < 0)
    {
        return recv_len;
    }

    ret_len = net_task_submit(sock_recvfrom, &param);
    if (ret_len < 0)
    {

        return -1;
    }
    recv_len += ret_len;

    return recv_len;
}

int sys_setsockopt(int sockfd, int level, int optname, const char *optval, int optlen)
{
    sock_setsockopt_param_t param;
    param.sockfd = sockfd;
    param.level = level;
    param.optname = optname;
    param.optval = optval;
    param.optlen = optlen;

    int ret;
    ret = net_task_submit(sock_setsockopt, &param);
    return ret;
}

int sys_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    sock_bind_param_t param;
    param.sockfd = sockfd;
    param.addr = addr;
    param.addrlen = addrlen;

    int ret;
    ret = net_task_submit(sock_bind,&param);
    return ret;
}

int sys_closesocket(int sockfd)
{
    int ret;
    ret = net_task_submit(sock_closesocket, sockfd);
    return ret;
}