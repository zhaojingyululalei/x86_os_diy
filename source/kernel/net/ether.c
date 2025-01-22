#include "ether.h"
#include "mem/malloc.h"
static void ether_show_head(ether_header_t* head)
{
    ETHER_DBG_PRINT("ether_header----\r\n");
    char src_buf[20]={0};
    char dest_buf[20] = {0};
    hwaddr_t src_mac;
    hwaddr_t dest_mac;
    memcpy(src_mac.addr,head->src,MACADDR_ARRAY_LEN);
    memcpy(dest_mac.addr,head->dest,MACADDR_ARRAY_LEN);
    mac_n2s(&src_mac,src_buf);
    mac_n2s(&dest_mac,dest_buf);
    uint16_t proto = ntohs(head->protocal);
    ETHER_DBG_PRINT("mac src:%s\r\n",src_buf);
    ETHER_DBG_PRINT("mac dest:%s\r\n",dest_buf);
    ETHER_DBG_PRINT("protoal:0x%04x\r\n",proto);
    ETHER_DBG_PRINT("\r\n");
}
static void ether_show_pkg(pkg_t* package)
{
    ETHER_DBG_PRINT("ether in a package:\r\n");
    ether_header_t *ether_head = package_data(package,sizeof(ether_header_t),0);
    ether_show_head(ether_head);
    package_print(package,sizeof(ether_header_t));

}
static int ether_pkg_is_ok(netif_t *netif, ether_header_t *header, int pkg_len)
{
    if (pkg_len > sizeof(ether_header_t) + MTU_MAX_SIZE)
    {
        dbg_warning("ether pkg len over size\r\n");
        return -1;
    }

    if (pkg_len < sizeof(ether_header_t))
    {
        dbg_warning("ether pkg len so small\r\n");
        return -2;
    }
    //既不是本机mac地址，也不是广播地址
    if (memcmp(netif->hwaddr.addr, header->dest, MACADDR_ARRAY_LEN) != 0 && memcmp(get_mac_broadcast()->addr, header->dest, MACADDR_ARRAY_LEN) != 0)
    {
        dbg_warning("ether recv a pkg,dest mac addr wrong\r\n");
        return -3;
    }
    return 0;
}
static int ether_open(netif_t* netif)
{
    return 0;
}
static void ether_close(netif_t* netif)
{

}
static int ether_in(netif_t* netif,pkg_t* package)
{
    
    ether_header_t *ether_head = package_data(package,sizeof(ether_header_t),0);
    if(ether_pkg_is_ok(netif,ether_head,package->total)<0)
    {
        dbg_warning("recv a wrong fomat package in ether layer\r\n");
        return -1;
    }
    
    return 0;
}
static int ether_out(netif_t * netif,ipaddr_t* dest,pkg_t* package)
{
    return 0;
}
const link_layer_t ether_ops = {
    .type = NETIF_TYPE_ETH,
    .open = ether_open,
    .close = ether_close,
    .in = ether_in,
    .out = ether_out
};