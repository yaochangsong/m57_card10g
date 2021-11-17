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

load_default_route()
{
    ifname=(`ifconfig |grep ^[a-z]|awk  '{print $1}'`)
    for((i=0;i<${#ifname[@]};i++))
    do
        gw=`/etc/netset.sh  get_gw ${ifname[i]}`
	if [ -z "$gw" ] || [ "${ifname[i]}" == "lo" ] || [ "${ifname[i]}" == "docker0" ]; then
                continue #echo "$gw is null or ${ifname[i]} is not allowed"
        else
                echo "set gateway ${ifname[i]}: $gw"
                isValidIp $gw
                route add default gw $gw
        fi
    done
}


load_default_route
