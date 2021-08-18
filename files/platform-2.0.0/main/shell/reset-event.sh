#!/bin/sh

MOUNT_DIR="/mnt"
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
factory_setting()
{
#    factory_config=/etc/.config_default.json
#    if [ -f "$factory_config" ]; then
#		cp -f /etc/config.json  /etc/.config.json.tmp
#        cp -f $factory_config /etc/config.json
#    fi
    
    factory_network=/etc/network/.interfaces_default
    if [ -f "$factory_network" ]; then
		cp /etc/network/interfaces /etc/network/.interfaces.tmp
		#若有(config)配置分区，原始网络配置文件存在配置分区中
		is_mount=$(mount |grep $MOUNT_DIR|grep -v grep|wc -l)
		if [ $is_mount -eq 0 ]; then #无配置分区
        cp $factory_network /etc/network/interfaces
		else #有配置分区
			cp $factory_network /$MOUNT_DIR/network/interfaces
		fi     
    fi
    xjson -f /etc/config.json -d 8866,8867,8868 -c 8686
}

param=$1
if [ "$param" == "reset_config" ] ;then
    echo "factory setting..."
    factory_setting
	restart_network
    echo "platform restart..."
    /etc/init.d/platform.sh restart
fi
