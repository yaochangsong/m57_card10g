#!/bin/sh

INS_PLATFOR_DIR="/usr/bin/"
INS_SRC_DIR="/tmp/app_update"
install_platform()
{
	echo "install platform!"
	install -m 0755 ${INS_SRC_DIR}/app/platform ${INS_PLATFOR_DIR}
	#install -m 0755 app/shell/platform.sh /etc/init.d/
	install -m 0755 ${INS_SRC_DIR}/app/compile.info /etc/
	install -m 0755 ${INS_SRC_DIR}/app/platform_version /etc/
	
	#echo "install router table!"
	#install -m 0755 app/nr_config.json /etc/
	#install -m 0755 app/sroute ${INS_PLATFOR_DIR}
	
	#install -m 0755 app/shell/platform.sh /etc/init.d/
	#echo "install shell!"
	#install -m 0755 app/shell/*.sh /etc/
}

restart_platform()
{
	/etc/init.d/platform.sh restart
}

install_platform
restart_platform