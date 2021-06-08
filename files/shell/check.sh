#!/bin/sh

status_dir=/run/status
adc_status_file=$status_dir/adc
clk_status_file=$status_dir/clock
gps_status_file=$status_dir/gps
rf_status_file=$status_dir/rf
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
	adc_ok="false"
	gps_ok=$(cat $gps_status_file)
	rf_ok=$(cat $rf_status_file)
	inner_clk_stat=0
	inner_clk=0
	lock_stat=0
	lock_ok="no_locked"
	external_clk="inner_clock"
	reg_val=`devmem $ADC_STATE_REG_ADDR 32`
	let "adc_stat=$reg_val&0x03" 
	if [ $adc_stat -eq $((0x03)) ]; then
		log_out "adc status check ok!"
		adc_ok="ok"
	else
		log_out "adc status check faild!"
		adc_ok="false"
	fi
	echo -n $adc_ok > $adc_status_file
	
	let "inner_clk_stat=$reg_val&0x0200"
	if [ $inner_clk_stat -eq $((0x0200)) ]; then
		log_out "external clock"
		external_clk="external_clock"
	else
		log_out "internal clock"
		external_clk="inner_clock"
	fi

	let "lock_stat=$reg_val&0x0100"
	if [ $lock_stat -eq $((0x0100)) ]; then
		log_out "locked"
		lock_ok="locked"
	else
		log_out "no locked"
		lock_ok="no_locked"
	fi
	echo -n $external_clk $lock_ok > $clk_status_file
	
	if [ "$gps_ok" == "ok" ]; then
		log_out "GPS OK!"
	else
		log_out "GPS Faild!"
	fi
	if [ "$lock_ok" == "locked" ] && [ "$adc_ok" == "ok" ] && [ "$rf_ok" == "ok" ]; then
		log_out "ok status..."
		/etc/led.sh check_ok
	else
		log_out "faild status"
		/etc/led.sh check_faild
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
	if [ ! -d "$status_dir" ]; then
	  mkdir $status_dir
	fi
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
