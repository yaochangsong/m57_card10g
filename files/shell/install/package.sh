#!/bin/sh

INS_PLATFORM_DIR="/usr/bin/"

appdir=`pwd`/files

do_package()
{
	mkdir -p $appdir/build/app/shell
	jenkins_ver=$appdir/conf/compile.info
	pl_ver=$appdir/conf/platform_version
	logfile=$appdir/conf/rsyslog.d/platform.conf
	logrotate_file=$appdir/conf/logrotate/platform 
	
	cp -f $appdir/shell/install/install.sh $appdir/build/
	cp -f $jenkins_ver $appdir/build/app
	cp -f $pl_ver	$appdir/build/app
	cp -f $appdir/platform $appdir/build/app
	if [ ! -d rsyslog.d ]; then
		mkdir -p $appdir/build/app/rsyslog.d
	fi
	cp -f $logrotate_file $appdir/build/app/rsyslog.d
	cp -f $logfile $appdir/build/app/rsyslog.d
	cp -f $appdir/shell/*.sh	$appdir/build/app/shell/
	tar -zcf platform_install.tar.gz -C $appdir/build .
}

do_package
