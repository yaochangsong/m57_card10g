#!/bin/sh

factory_setting()
{
    factory_config=/etc/.config_default.json
    if [ -f "$factory_config" ]; then
		cp -f /etc/config.json  /etc/.config.json.tmp
        cp -f $factory_config /etc/config.json
    fi
    
    factory_network=/etc/network/.interfaces_default
    if [ -f "$factory_network" ]; then
		cp /etc/network/interfaces /etc/network/.interfaces.tmp
        cp $factory_network /etc/network/interfaces
    fi
}

param=$1
if [ "$param" == "reset_config" ] ;then
    echo "factory setting..."
    factory_setting
    echo "platform restart..."
    /etc/init.d/platform.sh restart
fi
