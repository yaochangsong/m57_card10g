#!/bin/sh

#set -e
DAEMON=/usr/bin/platform
start()
{
    echo " Start FlatForm."
    str=$(cat /etc/profile | grep 'export TZ')
    if [ -z "$str" ]; then
		echo "TZ=UTC-08:00" >> /etc/profile
		echo "export TZ" >> /etc/profile
    fi
    #udp send buffer 8m
    echo 8388608 > /proc/sys/net/core/wmem_max
    #printk
    echo 3 4 1 3 > /proc/sys/kernel/printk
    sleep 1
    start-stop-daemon -S -o --background -x $DAEMON
    sleep 1
    #daemon check
    is_running=$(ps -ef|grep "checkproc.sh"|grep -v grep|wc -l)
    if [ $is_running -eq 0 ]; then
	/etc/checkproc.sh start >/dev/null 2>&1
    fi
    is_running=$(ps -ef|grep "tcpsvd"|grep -v grep|wc -l)
    if [ $is_running -eq 0 ]; then
        /etc/ftp.sh start 2021 >/dev/null 2>&1
    fi
}


factory_setting()
{
    #factory_config=/etc/.config_default.json
    #if [ ! -f "$factory_config" ]; then
    #    cp -f /etc/config.json $factory_config
    #fi
    
	factory_network=/etc/network/.interfaces_default
	if [ ! -f "$factory_network" ]; then
        cp -f /etc/network/interfaces $factory_network
    fi

}

stop()
{
	killall checkproc.sh >/dev/null 2>&1
    if start-stop-daemon -K -x $DAEMON >/dev/null 2>&1; then
    	echo "Stop FlatForm..."
    fi
}

case $1 in
        start)
                factory_setting
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