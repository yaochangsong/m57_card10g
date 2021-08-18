#!/bin/sh

RAM_ROOT=/tmp/root

[ -x /usr/bin/ldd ] || ldd() { LD_TRACE_LOADED_OBJECTS=1 $*; }
libs() { ldd $* 2>/dev/null | sed -r 's/(.* => )?(.*) .*/\2/'; }

install_file() { # <file> [ <file> ... ]
    for file in "$@"; do
        dest="$RAM_ROOT/$file"
        [ -f $file -a ! -f $dest ] && {
            dir="$(dirname $dest)"
            mkdir -p "$dir"
            cp $file $dest
        }
    done
}

install_bin() { # <file> [ <symlink> ... ]
    src=$1
    files=$1
    [ -x "$src" ] && files="$src $(libs $src)"
    install_file $files
    shift
    for link in "$@"; do {
        dest="$RAM_ROOT/$link"
        dir="$(dirname $dest)"
        mkdir -p "$dir"
        [ -f "$dest" ] || ln -s $src $dest
    }; done
}

supivot() { # <new_root> <old_root>
    /bin/mount | grep "on $1 type" 2>&- 1>&- || /bin/mount -o bind $1 $1
    mkdir -p $1$2 $1/proc $1/sys $1/dev $1/tmp && \
    /bin/mount -o noatime,move /proc $1/proc && \
    pivot_root $1 $1$2 || {
        /bin/umount -l $1 $1
        return 1
    }

    /bin/mount -o noatime,move $2/sys /sys
    /bin/mount -o noatime,move $2/dev /dev
    /bin/mount -o noatime,move $2/tmp /tmp
    return 0
}
run_ramfs() { # <command> [...]
    install_file $1
    install_bin /bin/busybox /bin/ash /bin/sh     \
        /sbin/pivot_root /bin/sync /bin/dd /bin/grep       \
        /bin/cp /bin/mv /bin/tar /usr/bin/md5sum "/usr/bin/[" /bin/dd   \
        /bin/vi /bin/ls /bin/cat /usr/bin/awk /usr/bin/hexdump      \
        /bin/sleep /bin/zcat /usr/bin/bzcat /usr/bin/printf /usr/bin/wc \
        /bin/cut /usr/bin/printf /bin/sync /bin/mkdir /bin/rmdir    \
        /bin/rm /usr/bin/basename /bin/kill /bin/chmod /usr/bin/find \
        /bin/mknod
    install_bin /bin/busybox.suid /bin/mount
    install_bin /bin/busybox.nosuid /bin/umount /bin/echo /sbin/reboot
    install_bin /bin/uclient-fetch /bin/wget
    install_bin /sbin/mtd
    install_bin /sbin/mount_root
    install_bin /sbin/snapshot
    install_bin /sbin/snapshot_tool
    install_bin /usr/sbin/ubiupdatevol
    install_bin /usr/sbin/ubiattach
    install_bin /usr/sbin/ubiblock
    install_bin /usr/sbin/ubiformat
    install_bin /usr/sbin/ubidetach
    install_bin /usr/sbin/ubirsvol
    install_bin /usr/sbin/ubirmvol
    install_bin /usr/sbin/ubimkvol
    install_bin /usr/sbin/partx
    install_bin /usr/sbin/losetup
    install_bin /usr/sbin/mkfs.ext4
    install_bin /usr/sbin/flashcp
    install_bin /usr/sbin/flash_erase
    for file in $RAMFS_COPY_BIN; do
        install_bin ${file//:/ }
    done
    install_file /etc/resolv.conf /etc/*.json /lib/*.sh /lib/functions/*.sh /lib/upgrade/*.sh $RAMFS_COPY_DATA

    [ -L "/lib64" ] && ln -s /lib $RAM_ROOT/lib64

    supivot $RAM_ROOT /mnt 2>/dev/null || {
        echo "Failed to switch over to ramfs. Please reboot."
        exit 1
    }

    /bin/mount -o remount,ro /mnt 2>/dev/null
    /bin/umount -l /mnt 2>/dev/null
    #echo "starting upgrade rootfs..."
    # spawn a new shell from ramdisk to reduce the probability of cache issues
    exec /bin/busybox ash -c " /usr/sbin/flash_erase /dev/$2 0 0 ;/usr/sbin/flashcp $1 /dev/$2 ;cd /; /bin/mkdir /tmp/mtd; /bin/mount -t jffs2 /dev/mtdblock3 /tmp/mtd; /bin/mount -o remount,rw /tmp/mtd; /bin/cp -f /etc/*.json /tmp/mtd/etc/; /bin/sync; /bin/umount /dev/mtdblock3; reboot -f"
}

if [ $# != 2 ]; then
    echo "$0 rootfs.jffs2 mtd*"
    exit 0
fi

[ -f "$1" ] ||  { echo "no rootfs file" ;exit ; }
run_ramfs $1 $2>/dev/null 2>&1

