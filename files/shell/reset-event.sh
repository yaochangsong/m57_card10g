#!/bin/sh

load_default_1g_network (){
    DEFAULT_IP="192.168.1.111"
    DEFAULT_NETMASK="255.255.255.0"
    DEFAULT_GW="192.168.1.1"
    DEFAULT_PORT=1234

    xjson -w -f /etc/config.json -n ipaddress,network -v $DEFAULT_IP
    xjson -w -f /etc/config.json -n netmask,network -v $DEFAULT_NETMASK
    xjson -w -f /etc/config.json -n gateway,network -v $DEFAULT_GW
    xjson -w -f /etc/config.json -n port,network -v $DEFAULT_PORT    
}

load_default_10g_network (){
    DEFAULT_IP="192.168.2.111"
    DEFAULT_NETMASK="255.255.255.0"
    DEFAULT_GW="192.168.2.1"
    DEFAULT_PORT=1234

    xjson -w -f /etc/config.json -n ipaddress,network_10g -v $DEFAULT_IP
    xjson -w -f /etc/config.json -n netmask,network_10g -v $DEFAULT_NETMASK
    xjson -w -f /etc/config.json -n gateway,network_10g -v $DEFAULT_GW
    xjson -w -f /etc/config.json -n port,network_10g -v $DEFAULT_PORT    
}

param=$1
if [ "$param" == "reset_config" ] ;then
	load_default_1g_network
	load_default_10g_network
	/etc/init.d/platform.sh restart
fi
