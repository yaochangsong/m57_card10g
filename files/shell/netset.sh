#!/bin/sh

#netset.sh set eth0 192.168.2.111 255.255.255.0 192.168.2.1
#netset.sh get_ip eth0 
#netset.sh get_netmask eth0 
#netset.sh get_gw eth0 
interface=/etc/network/interfaces 

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
	
set_all()
{
    ifname=$1
	new_ipaddr=$2
	new_netmask=$3
	new_gw=$4
	ipaddr=$(grep -A 50  $ifname $interface|grep 'address'|sed -n '1p'|awk '{print $2}')
	netmask=$(grep -A 50  $ifname $interface|grep 'netmask'|sed -n '1p'|awk '{print $2}')
	gateway=$(grep -A 50  $ifname $interface|grep 'gateway'|sed -n '1p'|awk '{print $2}')
    isValidIp $ipaddr
    isValidIp $netmask
    isValidIp $gateway
    sed -i '/iface '"$ifname"'/,/address/s/'$ipaddr'/'$new_ipaddr'/g' $interface
	sed -i '/iface '"$ifname"'/,/netmask/s/'$netmask'/'$new_netmask'/g' $interface
	sed -i '/iface '"$ifname"'/,/gateway/s/'$gateway'/'$new_gw'/g' $interface
	ifconfig $ifname $new_ipaddr netmask $new_netmask
	ip route add default via $new_gw dev $ifname
}

get_ip()
{
    ifname=$1
    ipaddr=$(grep -A 50  $ifname $interface|grep 'address'|sed -n '1p'|awk '{print $2}')
    echo -n $ipaddr
}

get_netmask()
{
    ifname=$1
    netmask=$(grep -A 50  $ifname $interface|grep 'netmask'|sed -n '1p'|awk '{print $2}')
    echo -n $netmask
}

get_gw()
{
    ifname=$1
    gw=$(grep -A 50  $ifname $interface|grep 'gateway'|sed -n '1p'|awk '{print $2}')
    echo -n $gw
}

case $1 in
        set)
				isValidIp $3
				isValidIp $4
				isValidIp $5
	    	    set_all $2 $3 $4 $5
                ;;
        get_ip)
                get_ip $2
                ;;
        get_netmask)
                get_netmask $2
                ;;
		get_gw)
                get_gw $2
                ;;
        *)
                echo "Usage: $0 set|get_ip|get_netmask|get_gw eth0|1..." 
                echo "       $0 set eth0 192.168.2.111 255.255.255.0 192.168.2.1"
                echo "       $0 get_ip eth0"
                echo "       $0 get_netmask eth0"
                echo "       $0 get_gw eth0"
                exit 2
                ;;
esac
