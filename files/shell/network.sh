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
net_1g_mac="00:0A:35:00:12:AA"

net_10g_ip=`xjson -r -f /etc/config.json -n ipaddress,network_10g`
net_10g_netmask=`xjson -r -f /etc/config.json -n netmask,network_10g`
net_10g_gateway=`xjson -r -f /etc/config.json -n gateway,network_10g`
net_10g_mac="00:0A:35:00:12:AB"

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

