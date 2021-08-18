#!/bin/sh

MOUNT_DIR="/mnt"
CONFIG_MNT_PATH_NAME="$MOUNT_DIR/config.json"
CONFIG_PATH_NAME="/etc/config.json"
RAW_CONFIG_PATH_NAME="/etc/config.json.jffs2"

#配置是否有独立分区;打印提示
is_mount=$(mount |grep $MOUNT_DIR|grep -v grep|wc -l)
if [ $is_mount -eq 0 ];
then
    echo "the configuration partiton is not mounted!!!"
fi

#判断挂载点配置文件是否存在；不存在拷贝文件系统默认配置到挂载点路径;同时保存到挂载点独立分区
if [ ! -f $CONFIG_MNT_PATH_NAME ]; then
    echo "config.json is not find on partiton! make a copy!!"
	cp -f $RAW_CONFIG_PATH_NAME $CONFIG_MNT_PATH_NAME
	sync        
fi

ln -sf $CONFIG_MNT_PATH_NAME $CONFIG_PATH_NAME


