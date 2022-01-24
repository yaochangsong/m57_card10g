#!/bin/sh

work_dir="/etc/goahead"
web_dir=$work_dir/web
authfile=$work_dir/auth.txt
routefile=$work_dir/route.txt

start()
{
	echo "start goahead..."
	start-stop-daemon -S -o --background -x $work_dir/goahead -- -a $authfile -r $routefile $web_dir http://*:80 > /dev/null  2>/dev/null &  #|logger -it goahead -p local0.notice &
	#$work_dir/goahead -a $authfile -r $routefile $web_dir http://*:80 > /dev/null  2>/dev/null &
}

stop()
{
    if start-stop-daemon -K -x $work_dir/goahead >/dev/null 2>&1; then
        echo "Stop Goahead..."
    fi
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

