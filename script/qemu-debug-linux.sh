# 适用于Linux
#script=no 表示禁用QEMU自动配置 tap0 接口的脚本,自己去主机上创建tap接口并配置
sudo qemu-system-i386 -daemonize -m 128M -s -S  -drive file=disk1.img,index=0,media=disk,format=raw -drive file=disk2.img,index=1,media=disk,format=raw \
 -rtc base=localtime -netdev tap,id=mynetdev0,ifname=tap0,script=no -device rtl8139,netdev=mynetdev0,mac=52:54:00:c9:18:27 \
 

# -netdev tap,id=mynetdev0,ifname=tap0,script=no -device rtl8139,netdev=mynetdev0,mac=52:54:00:c9:18:27

