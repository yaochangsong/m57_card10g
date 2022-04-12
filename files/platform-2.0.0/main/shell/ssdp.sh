#!/bin/sh

JSON_CONF_FILE=../conf/config.json
[ ! -f $JSON_CONF_FILE ] && echo "file $JSON_CONF_FILE not exist!"|| echo "$JSON_CONF_FILE exist" > /dev/null 2>&1

if ! type jq > /dev/null 2>&1; then 
	echo "jq not install!"
	exit 1
fi

#read port
port=0

CONF_PATH=/tmp
[ ! -d $CONF_PATH ] && echo "path $CONF_PATH not exist!" > /dev/null 2>&1

SSDP_BIN=/usr/bin/ssdp
#[ -x $SSDP_BIN ] || echo "$SSDP_BIN not exist!"; exit 1

read_config_port()
{
	ifname=$1
	_cmd_port=`cat $JSON_CONF_FILE |jq --arg ifname "$ifname" '.network|=map(select(.ifname==$ifname))' |jq '.network[].port.command'|jq -c .`
	cmd_port="[$_cmd_port]";
	data_port=`cat $JSON_CONF_FILE |jq --arg ifname "$ifname" '.network|=map(select(.ifname==$ifname))' |jq '.network[].port.data'|jq -c .`
	echo "Port $cmd_port$data_port"
}

write_info()
{
	ifname=$1
	port=$2
	_conf_file=$CONF_PATH/${ifname}_conf.info
	[ ! -f $_conf_file ] && echo "Create file $_conf_file";touch $_conf_file || echo "$_conf_file exist!"
	echo "write port:$port to $_conf_file"
	echo $port > $_conf_file
}

loop_check()
{
	if [ ! -n "$2" ]; then
		interval=2
	else
		interval=$2
	fi
	if [ ! -n "$1" ]; then
		echo "ifname is not valid!"
		exit 1
	fi
	ifname=$1
	echo "check port channge, ifname:$ifname interval:$interval"
	while :
        do
        	sleep $interval
        	_port=`read_config_port $ifname`
		if [ "$_port" != "$port" ]; then
			port="$_port"
			write_info $ifname "$port"
			stop
			start
		fi	
        done
}

start()
{
	echo "Start $SSDP_BIN"
	start-stop-daemon -S -o --background -x $SSDP_BIN
}

stop()
{
	if start-stop-daemon -K -x $SSDP_BIN >/dev/null 2>&1; then
        	echo "Stop $SSDP_BIN..."
    	fi
}

case $1 in
        start)
                start &
                ;;
        stop)
                stop
                ;;
	loop)
		loop_check $2 $3
		;;
        restart|force-reload)
                $0 stop && sleep 2 && $0 start
                ;;
        *)
                echo "Usage: $0 {start|stop|restart|loop}"
                exit 2
                ;;
esac
