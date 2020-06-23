#!/bin/sh

start()
{
    echo " Start FlatForm."
	/etc/led.sh init
    /etc/led.sh power on
    /etc/network.sh
    /etc/led.sh work on
#   insmod /lib/modules/4.6.0-xilinx/extra/dmadrvm.ko
 #   insmod /lib/modules/4.6.0-xilinx/extra/xwfsm.ko
    platform &
    sleep 1
 #   ifconfig eth0 down                                                 
 #   ifconfig eth0 hw ether 02:0a:35:00:00:04                 
 #   ifconfig eth0 up
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
