#!/bin/sh

top=`pwd`
appdir=$top/files/platform-2.0.0/main

do_package()
{
	mkdir -p $appdir/build/app/shell
	jenkins_ver=$appdir/conf/compile.info
	pl_ver=$appdir/conf/platform_version
	logfile=$appdir/conf/rsyslog.d/platform.conf
	logrotate_file=$appdir/conf/logrotate/platform 
	cron_file=$appdir/conf/logrotate/platform.cron
	platform_conf_file=$appdir/conf/config.json
	etc_dir=$appdir/build/app/etc
	nrs1800_dir=$appdir/tools/nrs1800
	nrs1800_desc_dir=$appdir/tools/nrs1800_desc
	
	cp -f $appdir/shell/install/install.sh $appdir/build/
	cp -f $jenkins_ver $appdir/build/app
	cp -f $pl_ver	$appdir/build/app
	cp -f $top/files/platform-2.0.0/platform $appdir/build/app
	if [ ! -d rsyslog.d ]; then
		mkdir -p $appdir/build/app/rsyslog.d
	fi
	cp -f $logrotate_file $appdir/build/app/rsyslog.d
	cp -f $cron_file $appdir/build/app/rsyslog.d
	cp -f $logfile $appdir/build/app/rsyslog.d
	cp -f $appdir/shell/*.sh	$appdir/build/app/shell/

	if [ ! -d $etc_dir ]; then
		mkdir -p $etc_dir
	fi
	if [ -f $platform_conf_file ]; then
		cp -f $platform_conf_file $etc_dir/
	fi
	#nrs1800
	if [ -f $nrs1800_dir/nr_config.json ]; then
		cp -f $nrs1800_dir/nr_config.json $etc_dir/
	fi
	if [ -f $nrs1800_dir/sroute ]; then
		cp -f $nrs1800_dir/sroute $etc_dir/
	fi
	if [ -f $nrs1800_desc_dir/nrs1800_desc.json ]; then
		cp -f $nrs1800_desc_dir/nrs1800_desc.json $etc_dir/
	fi
	if [ -f $nrs1800_desc_dir/nrs1800_desc ]; then
		cp -f $nrs1800_desc_dir/nrs1800_desc $etc_dir/
	fi
	#network
	if [ -f $appdir/shell/interfaces ]; then
		cp -f $appdir/shell/interfaces	$appdir/build/app/shell/
	fi
	if [ -f $appdir/shell/rc.local ]; then
		cp -f $appdir/shell/rc.local	$appdir/build/app/shell/
	fi
	tar -zcf platform_install.tar.gz -C $appdir/build .
}

do_package
