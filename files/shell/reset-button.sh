#!/bin/sh

start()
{
    echo " Start reset-button ..."
    reset-button >/dev/null
}
stop()
{
        echo " Stop reset-button"
        killall reset-button
}
restart()
{
        stop
        start
}
case "$1" in
    start)
        start
        ;;
    stop)
        stop
         ;;
    restart)
        restart
         ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
esac

exit $?
