#!/bin/sh

isValidMac()
{
	echo $1 > macfile
	mac=`grep -o -E '([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}' macfile`
	rm macfile
	if [ ${#mac} -eq 17 ]; then
		echo 1
	else
		echo 0
	fi
}

get_bootenv_mtd_name()
{
	cat /proc/mtd |grep bootenv|awk '{print $1}'|cut -b -4
	#echo "mtd1"
}

get_bootenv_size()
{
	mtd_name=`get_bootenv_mtd_name`> /dev/null
	size=`mtd_debug info /dev/$mtd_name|grep mtd.size|awk '{print $3}'` > /dev/null
	#size=1234
	#检测是否为数字
	if [ $size -gt 0 ] 2>/dev/null; then
		echo $size
	else
		echo 0
	fi
}

get_env_mac()
{
	DEFAULT_MAC="00:0A:35:00:12:34"
	local size
	local env
	local macstr
	local ret=0

	size=`get_bootenv_size`
	if [ "$size" != "0" ]; then
		mtd_name=`get_bootenv_mtd_name`
		mtd_debug read  /dev/$mtd_name 0 $size bootenv > /dev/null 2>&1
		env=`cat -b bootenv` > /dev/null 2>&1
		macstr=`echo $env|awk -F 'ethaddr=' '{print $2}'|cut -b -17` > /dev/null 2>&1
		rm bootenv
		ret=`isValidMac $macstr`
		if [ $ret -eq 1 ]; then
			echo "$macstr"
		else
			echo "$DEFAULT_MAC"
		fi
	fi
}

mac_gen()
{
	count=1
	macstr=$1 
	isValidMac $macstr  > /dev/null
	last_mac=${macstr:15:2}
	middle_mac=${macstr:12:2}
	head_mac="${macstr:0:12}"
	macdec=$( printf "%d\n" 0x$middle_mac)
	macdec1=$( expr $macdec + $count ) # to subtract one
	if [ $macdec1 -gt 255 ]; then
			macdec1=$( expr $macdec1 - 255 )
	fi
	machex1=$(echo $macdec1 |awk '{printf("%02x\n", $0)}') # to convert to hex again
	mac_md="$head_mac""$machex1":"$last_mac"
	isValidMac $mac_md  > /dev/null
	echo "$mac_md"
}

env_mac=`get_env_mac`
mac=`mac_gen $env_mac` > /dev/null 2>&1

echo $mac
