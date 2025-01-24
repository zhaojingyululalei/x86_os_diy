#ifndef __IPV4_H
#define __IPV4_H
#include "debug.h"
#include "types.h"
#include "net_cfg.h"
#include "netif.h"
#include "protocal.h"
#include "ipaddr.h"
#define IPV4_HEAD_VERSION   4
#define IPV4_HEAD_MIN_SIZE  20
#define IPV4_HEAD_MAX_SIZE  60
#define IPV4_HEAD_FLAGS_MORE_FRAGMENT    (1<<0)
#define IPV4_HEAD_FLAGS_NOT_FRAGMENT    (1<<1)
#define IPV4_HEAD_FLAGS_RESERVED_BIT    (1<<2)
#define IPV4_HEAD_TTL_DEFAULT   64
#define IPV4_FRAG_MAX       16

#pragma pack(1)
/*网络序，数据包中的东西统一使用网络序*/
typedef struct _ipv4_header_t
{
    uint8_t version_and_ihl; //包头长，单位4字节
    uint8_t DSCP6_and_ENC2;
    uint16_t total_len;
    uint16_t id;
    uint16_t frag_flags_and_offset; //offset 8字节为单位
    uint8_t ttl;
    uint8_t protocal;
    uint16_t h_checksum;
    uint32_t src_ip;
    uint32_t dest_ip;

}ipv4_header_t;

#pragma pack(0)


typedef struct _ipv4_head_parse_t
{
    uint8_t version;          // 版本号
    uint8_t head_len;              // 头部长度
    uint8_t dscp;             // 差分服务字段 (6位)
    uint8_t enc;              // 显式拥塞通知 (2位)
    uint16_t total_len;       // 总长度
    uint16_t id;              // 标识字段
    uint8_t flags;            // 分片标志 (高3位)
    uint16_t frag_offset;     // 分片偏移量 (低13位)
    uint8_t ttl;              // 生存时间
    uint8_t protocol;         // 协议号
    uint16_t checksum;        // 首部校验和
    ipaddr_t src_ip;          // 源IP地址 (主机字节序)
    ipaddr_t dest_ip;         // 目标IP地址 (主机字节序)
} ipv4_head_parse_t;

void parse_ipv4_header(const ipv4_header_t *ip_head, ipv4_head_parse_t *parsed);
void ipv4_set_header(const ipv4_head_parse_t* parsed, ipv4_header_t* head) ;

netif_t* ip_route(ipaddr_t* dest);

int ipv4_in(netif_t* netif,pkg_t* package);
int ipv4_out(pkg_t *package, protocal_type_t protocal,  ipaddr_t *dest);
void ipv4_init(void);


/*ip包分片处理*/
/*ip数据包分片结构*/
typedef struct _ip_frag_t
{
    ipaddr_t ip;
    uint16_t id;
    int tmo;      //tmo/period,每次扫描，tmo-1
    list_t frag_list; //分片数据包

    list_node_t node;
}ip_frag_t;

void ipv4_frag_list_print(void);
void ipv4_frag_free(ip_frag_t* frag);
ip_frag_t* ipv4_frag_alloc(void);
ip_frag_t *ipv4_frag_find(ipaddr_t *ip, uint16_t id);
int ipv4_frag_insert_pkg(ip_frag_t* frag,pkg_t* pkg,ipv4_head_parse_t* parse);
int ipv4_frag_is_all_arrived(ip_frag_t* frag);
pkg_t* ipv4_frag_join_pkg(ip_frag_t* frag);

/*debug*/
void ipv4_show_pkg(ipv4_head_parse_t *parse);
void ipv4_show_head(ipv4_header_t* head);
#ifdef IPV4_DBG
    #define IPV4_DBG_PRINT(fmt, ...) dbg_info(fmt, ##__VA_ARGS__)
#else
    #define IPV4_DBG_PRINT(fmt, ...) do { } while(0)
#endif
#endif