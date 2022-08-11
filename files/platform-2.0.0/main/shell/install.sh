#!/bin/sh

[ "$SRC_DIR" == "" ] && SRC_DIR=/tmp/app_update
[ "$DST_DIR" == "" ] && DST_DIR=/

install_pkt()
{
	echo "cp -rf $SRC_DIR/*	$DST_DIR"
	cp -rf $SRC_DIR/*	$DST_DIR
	rm $DST_DIR/install.sh
	sync
}

restart_platform()
{
	/etc/init.d/platform.sh restart
}

install_pkt
#restart_platform
