#!/bin/sh

INS_PLATFORM_DIR="/usr/bin/"

appdir=`pwd`/files

do_package()
{
	mkdir -p $appdir/build/app/shell
	jenkins_ver=$appdir/conf/compile.info
	pl_ver=$appdir/conf/platform_version
	
	cp -f $appdir/shell/install/install.sh $appdir/build/
	cp -f $jenkins_ver $appdir/build/app
	cp -f $pl_ver	$appdir/build/app
	cp -f $appdir/platform $appdir/build/app
	cp -f $appdir/shell/*.sh	$appdir/build/app/shell/
	cd $appdir/build
	tar -zcf platform_install.tar.gz *
}

do_package
