#!/bin/sh

power()
{
    echo "power $1"
    if [ "$1" == "on" ]; then
        echo "echo 0 > /sys/class/leds/led-power/brightness"
    else
        echo "echo 1 > /sys/class/leds/led-power/brightness"
    fi
}

work()
{
    echo "work $1"
    if [ "$1" == "on" ]; then
        echo "echo 0 > /sys/class/leds/led-work/brightness"
    else
        echo "echo 1 > /sys/class/leds/led-work/brightness"
    fi
}


transfer()
{
    echo "transfer $1"
    if [ "$1" == "on" ]; then
        echo "echo 0 > /sys/class/leds/led-transfer/brightness"
    else
        echo "echo 1 > /sys/class/leds/led-transfer/brightness"
    fi
}


check()
{
    echo "check $1"
    if [ "$1" == "on" ]; then
        echo "echo 0 > /sys/class/leds/led-check/brightness"
    else
        echo "echo 1 > /sys/class/leds/led-check/brightness"
    fi
}

case $1 in
        power)
                power $2
                ;;
        work)
                work $2
                ;;
        transfer)
                transfer $2
                ;;
        check)
                check $2
                ;;
        *)
                echo "Usage: $0 {power check transfer check on/off}"
                exit 2
                ;;
esac
