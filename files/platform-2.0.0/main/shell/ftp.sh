#!/bin/sh

DATA_DIR="/run/media/nvme0n1/data"
FILE_DIR="/etc/file"

start()
{
    echo "Start ftp server."
    local port=$1
    if [ ! -d $FILE_DIR ]; then
    	echo "File dir: $FILE_DIR is not exist!"
    	exit 1
    fi
    if [ -f "/usr/bin/tcpsvd" ] && [ -f "/usr/sbin/ftpd" ]; then
    	tcpsvd -vE 0.0.0.0 $port ftpd $FILE_DIR & >/dev/null 2>&1
    else
    	echo "not support ftp service!"
    	exit 1
    fi
}


stop()
{
	echo "Stop ftp server."
	killall tcpsvd >/dev/null 2>&1
}

case $1 in
        start)
	    	    start $2
                ;;
        stop)
                stop
                ;;
        restart)
                $0 stop && sleep 1 && $0 start $2
                ;;
        *)
                echo "Usage: $0 {start port|stop|restart port}"
                exit 2
                ;;
esac

