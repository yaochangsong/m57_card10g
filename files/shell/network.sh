#!/bin/sh -e

IFNAME_10G=eth0
cp /etc/network/interfaces /etc/network/.interfaces
mac=`/etc/genmac.sh`
/etc/netset.sh set_mac $IFNAME_10G $mac

restart_network()
{
    ifname=(`ifconfig |grep ^[a-z]|awk  '{print $1}'`)
    for((i=0;i<${#ifname[@]};i++))
    do
        ip addr flush dev ${ifname[$i]}
    done
    /etc/init.d/networking restart
}

ret=0
cmp -s /etc/network/interfaces /etc/network/.interfaces || ret=1                        
if [ $ret -ne 0 ]; then
	cp /etc/network/interfaces /etc/network/.interfaces
	echo "network restart..."
    restart_network
fi
