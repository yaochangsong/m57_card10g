#!/bin/sh

proj_name=check.sh
bin_name=platform
#devmem 0xb00010c0 32
ADC_STATE_REG_ADDR=0xb00010c0

get_timestamp()
{
    echo $(date "+%Y-%m-%d %H:%M:%S")
}

is_exist(){
    is_running=$(ps -ef|grep "$1"|grep -v grep|wc -l)
    if [ $is_running -eq 0 ]; then
        return 0
	else
		return 1
    fi
}

log_out()
{
	echo "$1" > /dev/null
}

self_status_cheak()
{
	#[15: 8] 
	#[1=ext ref 0=inner] [ref 1=lock 0=unlock]

	#[ 7: 0] 
	#[1= adc3 ok 0=error] [1= adc2 ok 0=error] [1= adc1 ok 0=error] [1= adc0 ok 0=error]
	adc_stat=0
	adc_ok=0
	inner_clk_stat=0
	lock_stat=0
	lock_ok=0
	reg_val=`devmem $ADC_STATE_REG_ADDR 32`
	let "adc_stat=$reg_val&0xff" 
	if [ $adc_stat -eq $((0x03)) ]; then
		log_out "adc status check ok!"
		adc_ok=1
	else
		log_out "adc status check faild!"
		adc_ok=0
	fi
	
	let "inner_clk_stat=$reg_val&0x0200"
	if [ $inner_clk_stat -eq $((0x0200)) ]; then
		log_out “external clock”
	else
		log_out “internal clock”
	fi

	let "lock_stat=$reg_val&0x0100"
	if [ $lock_stat -eq $((0x0100)) ]; then
		log_out “locked”
		lock_ok=1
	else
		log_out “no locked”
		lock_ok=0
	fi
	
	if [ $lock_ok -eq 1 ] && [ $adc_ok -eq 1 ]; then
		log_out "ok status..."
		/etc/led.sh check off
	else
		log_out "faild status"
		/etc/led.sh check off
	fi
	
	is_exist $bin_name
	if [ $? -eq 0 ]; then
		/etc/led.sh work off
	else
		/etc/led.sh work on
	fi
}

start()
{
	while :
	do
    	sleep 10
    	self_status_cheak
	done
}

stop()
{
	pid=`ps -ef|grep $1|grep -v grep|awk '{print $1}'`
	if [ "$pid" != "" ]; then
		log_out "kill[$1] pid:$pid"
		kill $pid
	fi
}

case $1 in
        start)
				start &
                ;;
        stop)
				is_exist "$proj_name start"
				if [ $? -eq 1 ]; then
					stop $proj_name&
				fi
                ;;
        restart|force-reload)
                $0 stop && sleep 2 && $0 start
                ;;
        *)
                echo "Usage: $0 {start|stop|restart}"
                exit 2
                ;;
esac
