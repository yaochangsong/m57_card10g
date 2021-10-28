#!/bin/sh

work_dir="/etc/goahead"
web_dir=$work_dir/web
authfile=$work_dir/auth.txt
routefile=$work_dir/route.txt

start()
{
	echo "start goahead..."
	$work_dir/goahead -a $authfile -r $routefile $web_dir http://*:9090 > /dev/null  2>/dev/null &
}

stop()
{
    echo "stop goahead..."
	killall goahead
}

case $1 in
        start)
                start &
                ;;
        stop)
                stop
                ;;
        restart|force-reload)
                $0 stop && sleep 1 && $0 start
                ;;
        *)
                echo "Usage: $0 {start|stop|restart}"
                exit 2
                ;;
esac

