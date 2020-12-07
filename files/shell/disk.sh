#!/bin/sh

DEV_NAME="/dev/nvme0n1"
MOUNT_DIR="/run/media/nvme0n1"
DATA_DIR="$MOUNT_DIR/data"
FILE_DIR="/etc/file"

d_mount()
{
    if [ ! -d $DATA_DIR ]; then
        mkdir -p $DATA_DIR
    fi
    mount $DEV_NAME $MOUNT_DIR
    ln -sfn $DATA_DIR $FILE_DIR
}

d_format()
{
    rm $FILE_DIR
    umount $MOUNT_DIR
    mkfs.ext2 $DEV_NAME
    d_mount
#    /etc/init.d/platform.sh restart &
}

case $1 in
    mount)
        d_mount
        ;;
    format)
        d_format
        ;;
    *)
        echo "Usage: $0 {mount|format}"
        exit 2
        ;;
esac
