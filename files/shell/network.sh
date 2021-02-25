#!/bin/sh -e

IFNAME=eth1
cp /etc/network/interfaces /etc/network/.interfaces
mac=`/etc/genmac.sh`
/etc/netset.sh set_mac $IFNAME $mac

ret=0
cmp -s /etc/network/interfaces /etc/network/.interfaces || ret=1                        
if [ $ret -ne 0 ]; then
	cp /etc/network/interfaces /etc/network/.interfaces
	echo "network restart..."
    ip addr flush dev $IFNAME
	ifconfig $IFNAME down
    /etc/init.d/networking restart
fi
