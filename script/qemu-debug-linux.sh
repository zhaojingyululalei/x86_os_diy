# 适用于Linux
qemu-system-i386 -daemonize -m 128M -s -S  -drive file=disk1.img,index=0,media=disk,format=raw -drive file=disk2.img,index=1,media=disk,format=raw \
-rtc base=localtime
