#!/bin/sh

start()
{
    echo " Start FlatForm."
    #LED灯控制
    /etc/led.sh init	>/dev/null 2>&1
    /etc/led.sh power on >/dev/null 2>&1
    /etc/led.sh check on >/dev/null 2>&1
    #网络初始化
    /etc/network.sh >/dev/null 2>&1
    /etc/led.sh work on	>/dev/null 2>&1
    #网络发送缓冲区最大修改位8m
    echo 8388608 > /proc/sys/net/core/wmem_max
    #磁盘挂载
    /etc/disk.sh mount
    #启动状态检测
    /etc/check.sh stop
    /etc/check.sh start
    #启动复位按钮检测
    /etc/reset-button.sh stop
    /etc/reset-button.sh start
    #调整内核部分打印级别
    echo 3 4 1 3 > /proc/sys/kernel/printk
    sleep 1
    platform & 
	#启动进程监控
	/etc/checkproc.sh stop
	/etc/checkproc.sh start
}

stop()
{
	echo " Stop FlatForm..."
	if killall platform > /dev/null 2>&1; then
		echo "killall platform"
	fi
    /etc/led.sh work off
}

case $1 in
        start)
		start
                ;;
        stop)
                stop
                ;;
        restart|force-reload)
                $0 stop && sleep 2 && $0 start
                ;;
        *)
                echo "Usage: $0 {start|stop|restart|try-restart|force-reload|status}"
                exit 2
                ;;
esac
