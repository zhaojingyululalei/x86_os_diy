#ifndef __NET_CFG_H
#define __NET_CFG_H
/*addr*/
#define IPADDR_ARRY_LEN 4
#define MACADDR_ARRAY_LEN 6

/*netif*/
#define NETIF_PKG_CACHE_CAPACITY 50


/*ether*/
#define ETHER_MTU_DEFAULT   1500
#define ETHER_MTU_MAX_SIZE ETHER_MTU_DEFAULT
#define ETHER_MTU_MIN_SIZE 46

/*arp*/
#define ARP_ENTRY_MAX_SIZE 20
#define ARP_ENTRY_PKG_CACHE_MAX_NR  5
//单位秒
#define ARP_ENTRY_TMO_STABLE    (20*60)    //已经解析的表项，每隔多久跟新一次
    //以下设置，arp表项处于wating状态最多15秒，否则就删除表项
#define ARP_ENTRY_TMO_RESOLVING 3    //该表项跟新时，每隔多久发一次arp请求
#define ARP_ENTRY_RETRY 5           //发了n次请求，还是没解析出来，就删除该表项

/*debug */
#define PKG_DBG
//#define NET_DRIVE_DBG
#define NETIF_DBG
#define ETHER_DBG
#define ARP_DBG
#define DBG_SOFT_TIMER_PRINT
#define IPV4_DBG
#endif