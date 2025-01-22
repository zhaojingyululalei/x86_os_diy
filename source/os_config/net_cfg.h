#ifndef __NET_CFG_H
#define __NET_CFG_H

#define IPADDR_ARRY_LEN 4
#define MACADDR_ARRAY_LEN 6
#define NETIF_PKG_CACHE_CAPACITY 50
#define ETHER_MTU_DEFAULT   1500
#define MTU_MAX_SIZE ETHER_MTU_DEFAULT

/**
 * debug 
 * 
 */
#define PKG_DBG
//#define NET_DRIVE_DBG
#define NETIF_DBG
#define ETHER_DBG
#endif