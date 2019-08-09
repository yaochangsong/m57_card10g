#!/bin/sh

start()
{
	echo " Start FlatForm."
	platform &
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
