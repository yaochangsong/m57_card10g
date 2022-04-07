#!/bin/sh

MOUNT_DIR="/mnt"
isreboot=0
network_config=/etc/network/interfaces
mount_network_config=$MOUNT_DIR/network/interfaces

restart_network()
{
    ifname=(`ifconfig |grep ^[a-z]|awk  '{print $1}'`)
    for((i=0;i<${#ifname[@]};i++))
    do
        ip addr flush dev ${ifname[$i]}
        ifconfig ${ifname[$i]} down
    done
    /etc/init.d/networking restart
}

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
#1st
load_network()
{
	ret=0
	if [ -f "$mount_network_config" ]; then
		cmp -s $network_config $mount_network_config  || ret=1
		if [ $ret -ne 0 ]; then
			echo "network parameter changed!"
			cp -f $network_config /etc/network/.interfaces.fs
			isreboot=1
		fi
		ln -fs $mount_network_config $network_config
		echo "ln -fs $mount_network_config $network_config"
	else
		#配置是否有独立分区;打印提示
		is_mount=$(mount |grep $MOUNT_DIR|grep -v grep|wc -l)
		if [ $is_mount -eq 0 ];
		then
			echo "the configuration partiton is not mounted!!!"
		else
			if [ ! -d $MOUNT_DIR/network ]; then
				mkdir $MOUNT_DIR/network
			fi
			cp -f $network_config $mount_network_config 
			ln -fs $mount_network_config $network_config
			echo "copy $network_config to $mount_network_config!"
			echo "ln -fs $mount_network_config $network_config"
		fi
    fi
}

#2nd
load_mac()
{
	ret=0
	IFNAME1G=eth1
	IFNAME10G=eth0
	cp /etc/network/interfaces /etc/network/.interfaces
	mac=`/etc/genmac.sh g1`
	/etc/netset.sh set_mac $IFNAME1G $mac
	mac=`/etc/genmac.sh g10`
	/etc/netset.sh set_mac $IFNAME10G $mac
	cmp -s /etc/network/interfaces /etc/network/.interfaces || ret=1    
	if [ $ret -ne 0 ]; then
        cp /etc/network/interfaces /etc/network/.interfaces
        isreboot=1
	fi
}

load_default_route()
{
    ifname=(`ifconfig |grep ^[a-z]|awk  '{print $1}'`)
    for((i=0;i<${#ifname[@]};i++))
    do
        gw=`/etc/netset.sh  get_gw ${ifname[i]}`
        if [ -n "$gw" ]; then
            isValidIp $gw
            route add default gw $gw
        fi
    done
}

#
load_manage_ipaddr()
{
    IFNAME=eth1
    ifconfig $IFNAME:0 192.168.188.254
}
load_network
load_mac
if [ $isreboot -ne 0 ]; then
    echo "network restart..."
    restart_network
fi
load_default_route
load_manage_ipaddr
