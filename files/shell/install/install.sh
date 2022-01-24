#!/bin/sh

INS_PLATFOR_DIR="/usr/bin/"
INS_SRC_DIR="/tmp/app_update"
LOG_DIR=/etc/rsyslog.d
LOGROTATE_DIR=/etc/logrotate.d
LOG_FILE=$LOG_DIR/platform.conf
LOGROTATE_FILE=$LOGROTATE_DIR/platform
LOG_CONF_SRC_FILE=${INS_SRC_DIR}/app/rsyslog.d/platform.conf
LOGROTATE_SRC_FILE=${INS_SRC_DIR}/app/rsyslog.d/platform
CRON_SRC_FILE=${INS_SRC_DIR}/app/rsyslog.d/platform.cron
GOAHEAD_SHELL=${INS_SRC_DIR}/app/shell/goahead.sh
install_platform()
{
	echo "install platform!"
	install -m 0755 ${INS_SRC_DIR}/app/platform ${INS_PLATFOR_DIR}
	install -m 0755 ${INS_SRC_DIR}/app/shell/platform.sh /etc/init.d/
	install -m 0755 ${INS_SRC_DIR}/app/compile.info /etc/
	install -m 0755 ${INS_SRC_DIR}/app/platform_version /etc/
	
	if [ -f "$LOG_CONF_SRC_FILE" ]; then
		install -m 0644 $LOG_CONF_SRC_FILE $LOG_FILE
		service rsyslog restart
	fi
	if [ -f "$LOGROTATE_SRC_FILE" ]; then
		install -m 0644 $LOGROTATE_SRC_FILE $LOGROTATE_FILE 
	fi
	if [ -f "$CRON_SRC_FILE" ]; then
		crontab $CRON_SRC_FILE
	fi
	
	if [ -f "$GOAHEAD_SHELL" ]; then
		install -m 0755 $GOAHEAD_SHELL /etc/init.d/
	fi	
	#echo "install router table!"
	#install -m 0755 ${INS_SRC_DIR}/app/nr_config.json /etc/
	#install -m 0755 ${INS_SRC_DIR}/app/sroute ${INS_PLATFOR_DIR}
	
	#install -m 0755 ${INS_SRC_DIR}/app/shell/platform.sh /etc/init.d/
	#echo "install shell!"
	#install -m 0755 ${INS_SRC_DIR}/app/shell/*.sh /etc/
	sync
}

restart_platform()
{
	/etc/init.d/platform.sh restart
}

install_platform
restart_platform
