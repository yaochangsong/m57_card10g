#!/bin/sh

start()
{
	echo " Start FlatForm."
    echo 30 > /sys/bus/iio/devices/iio\:device2/in_voltage0_hardwaregain 
    echo 30 > /sys/bus/iio/devices/iio\:device2/in_voltage1_hardwaregain
	platform &
    sleep 1
    ifconfig eth0 down                                                 
    ifconfig eth0 hw ether 02:0a:35:00:00:04                 
    ifconfig eth0 up
}

stop()
{
	echo " Stop FlatForm..."
	killall platform
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
