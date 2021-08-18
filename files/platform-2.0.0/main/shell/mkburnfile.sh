#!/bin/sh
IFS='='

config_dir=$1
ubootname=""
ubootsize=0
bootenvname=""
bootenvsize=0
kernelname=""
kernelsize=0
fsname=""
fssize=0
dtbname=""
dtbsize=0

bootfile="BOOT.BIN"
kernelfile="image.ub"
jffs2file="rootfs.jffs2"
dtbfile="system.dtb"

output_file="burnfile.bin"
config_file="../../project-spec/configs/config"
#config_file="config"
echo "---------------------------------------------------------"
if [ "$config_dir" != "" ]; then
    config_file="$config_dir/project-spec/configs/config"
    bootfile="$config_dir/images/linux/BOOT.BIN"
    kernelfile="$config_dir/images/linux/image.ub"
    jffs2file="$config_dir/images/linux/rootfs.jffs2"
    dtbfile="$config_dir/images/linux/system.dtb"
    output_file="$config_dir/images/linux/burnfile.bin"
fi
block_size=1024

if [ ! -f $bootfile ]; then
    echo ">>>>$bootfile File not found!"
    exit 1
fi

if [ ! -f $kernelfile ]; then
    echo ">>>>$kernelfile File not found!"
    exit 1
fi


if [ ! -f $jffs2file ]; then
    echo ">>>>$jffs2file File not found!"
    exit 1
fi


if [ ! -f $dtbfile ]; then
    echo ">>>>$dtbfile File not found!"
    exit 1
fi


while read k v
    do
        if [ -n "$v" ]; then
            if [ "$v" == "\"boot\"" ]; then
                ubootname=${k/_NAME/_SIZE}
            elif [ "$v" == "\"bootenv\""  ]; then
                bootenvname=${k/_NAME/_SIZE}
            elif [ "$v" == "\"kernel\"" ]; then
                kernelname=${k/_NAME/_SIZE}
            elif [ "$v" == "\"jffs2\"" ]; then
                fsname=${k/_NAME/_SIZE}
            elif [ "$v" == "\"dtb\"" ]; then
                dtbname=${k/_NAME/_SIZE}
            fi

            if [ "$k" == "$ubootname" ]; then
                ubootsize=$v
            elif [ "$k" == "$bootenvname" ]; then
                bootenvsize=$v
            elif [ "$k" == "$kernelname" ]; then
                kernelsize=$v
            elif [ "$k" == "$fsname" ]; then
                fssize=$v
            elif [ "$k" == "$dtbname" ]; then
                dtbsize=$v
            fi
        fi
done  < $config_file
echo "ubootsize=$ubootsize,bootenvsize=$bootenvsize, kernelsize=$kernelsize, fssize=$fssize,dtbsize=$dtbsize"
((ubootsize=$ubootsize))
((bootenvsize=$bootenvsize))
((kernelsize=$kernelsize))
((fssize=$fssize))
((dtbsize=$dtbsize))

ubootsize=`expr $ubootsize / $block_size`
bootenvsize=`expr $bootenvsize / $block_size`
kernelsize=`expr $kernelsize / $block_size`
fssize=`expr $fssize / $block_size`
dtbsize=`expr $dtbsize / $block_size`

if [ $ubootsize -eq 0 ] || [ $bootenvsize -eq 0 ] || [ $kernelsize -eq 0 ] || [ $fssize -eq 0 ] || [ $dtbsize -eq 0 ]; then
    echo ">>>>Make burn file Faild!"
fi
uboot_start=0
kernel_start=`expr $uboot_start + $ubootsize + $bootenvsize`
dtb_start=`expr $kernel_start + $kernelsize`
fs_start=`expr $dtb_start + $dtbsize`
echo "uboot_start=$uboot_start, kernel_start=$kernel_start, dtb_start=$dtb_start, fs_start=$fs_start"

ret=0
if [ $bootfile != "" ]; then
    echo "dd if=$bootfile of=$output_file bs=$block_size seek=$uboot_start"
    dd if=$bootfile of=$output_file bs=$block_size seek=$uboot_start
    if [ $? -ne 0 ]; then
        ret=1
    fi
fi	

if [ $kernelfile != "" ]; then
    echo "dd if=$kernelfile of=$output_file bs=$block_size seek=$kernel_start"
	dd if=$kernelfile of=$output_file bs=$block_size seek=$kernel_start
    if [ $? -ne 0 ]; then
        ret=1
    fi
fi

if [ $dtbfile != "" ]; then
    echo "dd if=$dtbfile of=$output_file bs=$block_size seek=$dtb_start"
	dd if=$dtbfile of=$output_file bs=$block_size seek=$dtb_start
    if [ $? -ne 0 ]; then
        ret=1
    fi
fi

if [ $jffs2file != "" ]; then
    echo "dd if=$jffs2file of=$output_file bs=$block_size seek=$fs_start"
    dd if=$jffs2file of=$output_file bs=$block_size seek=$fs_start
    if [ $? -ne 0 ]; then
        ret=1
    fi
fi
sync

echo "---------------------------------------------------------"
if  [ $ret -eq 0 ]; then
    echo ">>>>Make burn file $output_file OK!"
else
    echo ">>>>Make burn file Faild!"
fi
