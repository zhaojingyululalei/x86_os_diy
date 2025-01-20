#ifndef __NET_SYSCALL_H
#define __NET_SYSCALL_H
#include "netif.h"


int sys_netif_create(const char *if_name, netif_type_t type);
int sys_netif_open(const char *if_name);
int sys_netif_active(const char *if_name);
int sys_netif_deactive(const char *if_name);
int sys_netif_close(const char *if_name);
#endif
