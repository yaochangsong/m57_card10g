#!/bin/sh

proc_dir=/usr/bin
proc_name=platform
proc_start_cmd="/etc/init.d/$proc_name.sh start"
log_file="/var/log/checkproc.log"

get_timestamp()
{
    echo $(date "+%Y-%m-%d %H:%M:%S")
}

restart_process_if_die()
{
    is_running=$(ps -ef|grep $1|grep -v grep|wc -l)
    if [ $is_running -eq 0 ];
    then
        #echo "$(get_timestamp) $1 is down, now I will restart it" |tee -a $log_file
        logger -i -t "restart" -p local0.notice ">>>$(get_timestamp) $1 is down, now I will restart it<<<"
	echo "run start cmd: $proc_start_cmd"
        $proc_start_cmd
    fi
}

start()
{
	while :
	do
    	sleep 2
    	restart_process_if_die $proc_dir/$proc_name
	done
}

stop()
{
    killall checkproc.sh
    echo "stop..."
}

case $1 in
        start)
                start &
                ;;
        stop)
                stop
                ;;
        restart|force-reload)
                $0 stop && sleep 2 && $0 start
                ;;
        *)
                echo "Usage: $0 {start|stop|restart}"
                exit 2
                ;;
esac
