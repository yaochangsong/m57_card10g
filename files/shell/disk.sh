#!/bin/sh

DEV_NAME="/dev/nvme0n1"
MOUNT_DIR="/etc/file"

mount()
{
	if [ ! -d ]; then
		mkdir -p $MOUNT_DIR  >/dev/null 2>&1
	fi
    mount $DEV_NAME $MOUNT_DIR >/dev/null 2>&1
}

format()
{
	echo "Format,Please Wait...!!"
	umount $MOUNT_DIR
	mkfs.ext2 $DEV_NAME
	mount
}

case $1 in
        mount)
				mount
                ;;
        format)
                format
                ;;
        *)
                echo "Usage: $0 {mount|format}"
                exit 2
                ;;
esac
