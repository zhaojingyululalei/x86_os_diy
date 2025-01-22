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


/*debug */
#define PKG_DBG
//#define NET_DRIVE_DBG
#define NETIF_DBG
#define ETHER_DBG
#define ARP_DBG
#define DBG_SOFT_TIMER_PRINT
#endif