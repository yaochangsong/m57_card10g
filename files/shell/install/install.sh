#!/bin/sh

INS_PLATFORM_DIR="/usr/bin/"
INS_MODULE_DIR="/lib/modules/4.4.131-20191204.kylin.desktop-generic/extra/"

install_platform()
{
	echo "install platform!"
	install -m 0755 app/platform ${INS_PLATFORM_DIR}
	install -m 0755 app/shell/platform.sh /etc/init.d/
	
	#echo "install router table!"
	#install -m 0755 app/nr_config.json /etc/
	#install -m 0755 app/sroute ${INS_PLATFORM_DIR}
	
	echo "install shell!"
	install -m 0755 app/shell/*.sh /etc/
}

install_platform
