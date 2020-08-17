#!/bin/sh -e
### BEGIN INIT INFO
# Provides:          networking 
# Required-Start:    
# Required-Stop:     
# Default-Start:     
# Default-Stop:      
# Short-Description: 
# Description:       
### END INIT INFO
								  
#网络参数
net_1g_ip=`xjson -r -f /etc/config.json -n ipaddress,network`
net_1g_netmask=`xjson -r -f /etc/config.json -n netmask,network`
net_1g_gateway=`xjson -r -f /etc/config.json -n gateway,network`
net_1g_mac=""

net_10g_ip=`xjson -r -f /etc/config.json -n ipaddress,network_10g`
net_10g_netmask=`xjson -r -f /etc/config.json -n netmask,network_10g`
net_10g_gateway=`xjson -r -f /etc/config.json -n gateway,network_10g`
net_10g_mac=""

isValidIp() 
{
	local ip=$1
	local ret=1
	
	if [[ $ip =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
		ip=(${ip//\./ })
		[[ ${ip[0]} -le 255 && ${ip[1]} -le 255 && ${ip[2]} -le 255 && ${ip[3]} -le 255 ]]
		ret=$?
	fi
	if [ $ret -ne 0 ]; then
		echo "$1 is not valid address"
		exit 1
	fi
}

#检测mac有效性"xx:xx:xx:xx:xx:xx"
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

get_1g_mac()
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
		env=`cat bootenv` > /dev/null 2>&1
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

mac_add_with_parameter()
{
	count=1
	macstr=$1 #"f8:aa:bb:cc:ff:ff"
	isValidMac $macstr  > /dev/null
	last_mac=${macstr:15:2}
	head_mac="${macstr:0:15}"
	macdec=$( printf "%d\n" 0x$last_mac)
	macdec1=$( expr $macdec + $count ) # to subtract one
	if [ $macdec1 -gt 255 ]; then
			macdec1=$( expr $macdec1 - 255 )
	fi
	machex1=$(echo $macdec1 |awk '{printf("%02x\n", $0)}') # to convert to hex again
	mac_md="$head_mac""$machex1"
	echo "$mac_md"
}

mac_add_with_parameter_middle()
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

get_10g_mac()
{
	#net_10g_mac=`mac_add_with_parameter $net_1g_mac` > /dev/null 2>&1
	net_10g_mac=`mac_add_with_parameter_middle $net_1g_mac` > /dev/null 2>&1
	echo $net_10g_mac
}

net_1g_mac=`get_1g_mac`
net_10g_mac=`get_10g_mac`

#isValidIp $net_1g_ip
isValidIp $net_1g_ip
isValidIp $net_1g_netmask
isValidIp $net_1g_gateway
isValidIp $net_10g_ip
isValidIp $net_10g_netmask
isValidIp $net_10g_gateway


	cat >/etc/network/.interfaces <<EOF
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet static
address $net_10g_ip
netmask $net_10g_netmask
gateway $net_10g_gateway
hwaddress ether $net_1g_mac
mtu 9000

auto eth1
iface eth1 inet static
address $net_1g_ip
netmask $net_1g_netmask
gateway $net_1g_gateway
hwaddress ether $net_10g_mac
EOF

ret=0
#比较文件；不相等则重启网络
cmp -s /etc/network/interfaces /etc/network/.interfaces || ret=1                        
if [ $ret -ne 0 ]; then
	cp /etc/network/.interfaces /etc/network/interfaces
	echo "network restart..."
    ip addr flush dev eth0
    ip addr flush dev eth1
    /etc/init.d/networking restart
fi

