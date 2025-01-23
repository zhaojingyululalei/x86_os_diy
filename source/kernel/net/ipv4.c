#include "ipv4.h"
#include "ipaddr.h"
#include "algrithem.h"
#include "protocal.h"
void parse_ipv4_header(const ipv4_header_t *ip_head, ipv4_head_parse_t *parsed)
{
    // 解析字段
    parsed->version = (ip_head->version_and_ihl >> 4) & 0x0F;
    parsed->head_len = (ip_head->version_and_ihl & 0x0F) * 4;

    parsed->dscp = (ip_head->DSCP6_and_ENC2 >> 2) & 0x3F;
    parsed->enc = ip_head->DSCP6_and_ENC2 & 0x03;

    parsed->total_len = ntohs(ip_head->total_len);
    parsed->id = ntohs(ip_head->id);

    uint16_t frag_flags_and_offset_host = 0;
    // 将网络字节序转换为主机字节序
    frag_flags_and_offset_host = ntohs(ip_head->frag_flags_and_offset);
    // 提取 flags 和 frag_offset
    parsed->flags = (frag_flags_and_offset_host >> 13) & 0x07;       // 高3位为 flags
    parsed->frag_offset = (frag_flags_and_offset_host & 0x1FFF) * 8; // 低13位为 frag_offset

    parsed->ttl = ip_head->ttl;
    parsed->protocol = ip_head->protocal;

    parsed->checksum = ntohs(ip_head->h_checksum);
    parsed->src_ip.q_addr = ntohl(ip_head->src_ip);
    parsed->dest_ip.q_addr = ntohl(ip_head->dest_ip);
}
void ipv4_set_header(const ipv4_head_parse_t *parsed, ipv4_header_t *head)
{
    // 将 version 和 head_len 合并到 version_and_ihl
    head->version_and_ihl = ((parsed->version & 0x0F) << 4) | ((parsed->head_len / 4) & 0x0F);

    // DSCP 和 ENC2
    head->DSCP6_and_ENC2 = ((parsed->dscp & 0x3F) << 2) | (parsed->enc & 0x03);

    // 总长度（转换为网络字节序）
    head->total_len = htons(parsed->total_len);

    // 标识字段（转换为网络字节序）
    head->id = htons(parsed->id);

    // flags 和 fragment offset 合并后转换为网络字节序
    uint16_t frag_flags_and_offset_host = ((parsed->flags & 0x07) << 13) | (parsed->frag_offset & 0x1FFF);
    head->frag_flags_and_offset = htons(frag_flags_and_offset_host);

    // TTL 和协议
    head->ttl = parsed->ttl;
    head->protocal = parsed->protocol;

    // 校验和（转换为网络字节序）
    head->h_checksum = htons(parsed->checksum);

    // 源地址和目标地址（转换为网络字节序）
    head->src_ip = htonl(parsed->src_ip.q_addr);
    head->dest_ip = htonl(parsed->dest_ip.q_addr);
}

static bool ipv4_is_ok(ipv4_header_t* ip_head,ipv4_head_parse_t *parse)
{
    if (parse->version != IPV4_HEAD_VERSION)
    {
        return false;
    }
    if (parse->head_len < IPV4_HEAD_MIN_SIZE || parse->head_len > IPV4_HEAD_MAX_SIZE)
    {
        return false;
    }
    if (parse->head_len != sizeof(ipv4_header_t))
    {
        // 有额外数据的ip包暂时不处理
        return false;
    }
    if (parse->total_len < parse->head_len)
    {
        return false;
    }
    // 校验和为0时，即为不需要检查检验和
    if (parse->checksum)
    {
        uint16_t c = checksum16(0, (uint16_t *)ip_head, parse->head_len, 0, 1);
        if (c != 0)
        {
            dbg_warning("Bad checksum: %0x(correct is: %0x)\n", parse->checksum, c);
            return 0;
        }
    }
    return true;
}
static bool ipv4_is_match(netif_t *netif, ipaddr_t* dest_ip)
{
    // 本机ip，局部广播,255.255.255.255 都是匹配的
    

    // 是否是255.255.255.255
    if (is_global_boradcast(dest_ip))
    {
        return true;
    }
    // 看看主机部分是否为全1
    if (is_local_boradcast(&netif->info.ipaddr,&netif->info.mask, dest_ip))
    {
        return true;
    }
    if (netif->info.ipaddr.q_addr == dest_ip->q_addr)
    {
        return true;
    }
    return false;
}

/*无需分片的包*/
static int ipv4_normal_in(netif_t* netif,pkg_t* package,ipv4_head_parse_t* parse)
{
    ipv4_header_t* head = (ipv4_header_t*)package_data(package,sizeof(ipv4_header_t),0);
    switch (parse->protocol)
    {
    case PROTOCAL_TYPE_ICMPV4:
        
        break;
    case PROTOCAL_TYPE_UDP:
        dbg_info("udp handle:to be continued\r\n");
        break;
    case PROTOCAL_TYPE_TCP:
        dbg_info("tcp handle:to be continued\r\n");
        break;
    default:
        dbg_error("unkown ipv4 type\r\n");
        return -1;
    }
}
int ipv4_in(netif_t *netif, pkg_t *package)
{
    int ret;
    // 解析包头
    ipv4_header_t *ip_head = (ipv4_header_t *)package_data(package, sizeof(ipv4_header_t), 0);
    ipv4_head_parse_t parse_head;
    parse_ipv4_header(ip_head, &parse_head);
    ipv4_show_pkg(&parse_head);
    if (!ipv4_is_ok(ip_head,&parse_head))
    {
        dbg_warning("recv a wrong format ipv4 package\r\n");
        return -1;
    }
    if(!ipv4_is_match(netif,&parse_head.dest_ip))
    {
        dbg_warning("routing error:recv a pkg dest_ip not the host\r\n");
        return -2;
    }
    if (package->total > parse_head.total_len)
    {
        // ether 最小字节数46，可能自动填充了一些0
        package_shrank_last(package, package->total - parse_head.total_len);
    }

    ret = ipv4_normal_in(netif,package,&parse_head);
    if(ret < 0)
    {
        return -3;
    }
    //包被正确处理后，释放
    package_collect(package);
    return 0;
}

void ipv4_show_pkg(ipv4_head_parse_t *parse)
{
#ifdef IPV4_DBG
    dbg_info("++++++++++++++++++++show ipv4 header++++++++++++++++\r\n");
    dbg_info("version:%d\r\n", parse->version);
    dbg_info("head_len:%d\r\n", parse->head_len);
    dbg_info("dscp:%d\r\n", parse->dscp);
    dbg_info("enc:%d\r\n", parse->enc);
    dbg_info("total_len:%d\r\n", parse->total_len);
    dbg_info("id:0x%04x\r\n", parse->id);
    dbg_info("flags_1,Reserved bit:%x\r\n", (parse->flags & 0x04) ? 1 : 0);
    dbg_info("flags_2,Dont fragment:%x\r\n", (parse->flags & 0x02) ? 1 : 0);
    dbg_info("flags_3,More fragment:%x\r\n", (parse->flags & 0x01) ? 1 : 0);
    dbg_info("frag_offset:%d\r\n", parse->frag_offset);
    dbg_info("ttl:%d\r\n", parse->ttl);
    dbg_info("protocal:0x%02x\r\n", parse->protocol);
    dbg_info("checksum:0x%04x\r\n", parse->checksum);
    
    char src_buf[20] = {0};
    char dest_buf[20] = {0};
    ipaddr_n2s(&parse->src_ip, src_buf, 20);
    ipaddr_n2s(&parse->dest_ip, dest_buf, 20);
    dbg_info("src_ip:%s\r\n", src_buf);
    dbg_info("dest_ip:%s\r\n", dest_buf);
    dbg_info("++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
#endif
}
