#!/bin/sh -e
### BEGIN INIT INFO
# Provides:          networking ifupdown
# Required-Start:    mountkernfs $local_fs urandom
# Required-Stop:     $local_fs
# Default-Start:     S
# Default-Stop:      0 6
# Short-Description: Raise network interfaces.
# Description:       Prepare /run/network directory, ifstate file and raise network interfaces, or take them down.
### END INIT INFO

PATH="/sbin:/bin"
RUN_DIR="/run/network"
IFSTATE="$RUN_DIR/ifstate"
STATEDIR="$RUN_DIR/state"

[ -x /sbin/ifup ] || exit 0
[ -x /sbin/ifdown ] || exit 0

. /lib/lsb/init-functions

CONFIGURE_INTERFACES=yes
EXCLUDE_INTERFACES=
VERBOSE=no

[ -f /etc/default/networking ] && . /etc/default/networking

verbose=""
[ "$VERBOSE" = yes ] && verbose=-v

process_exclusions() {
    set -- $EXCLUDE_INTERFACES
    exclusions=""
    for d
    do
        exclusions="-X $d $exclusions"
    done
    echo $exclusions
}

process_options() {
    [ -e /etc/network/options ] || return 0
    log_warning_msg "/etc/network/options still exists and it will be IGNORED! Please use /etc/sysctl.conf instead."
}

check_ifstate() {
    if [ ! -d "$RUN_DIR" ] ; then
        if ! mkdir -p "$RUN_DIR" ; then
            log_failure_msg "can't create $RUN_DIR"
            exit 1
        fi
        if ! chown root:netdev "$RUN_DIR" ; then
            log_warning_msg "can't chown $RUN_DIR"
        fi
    fi
    if [ ! -r "$IFSTATE" ] ; then
        if ! :> "$IFSTATE" ; then
            log_failure_msg "can't initialise $IFSTATE"
            exit 1
        fi
    fi
}

check_network_file_systems() {
    [ -e /proc/mounts ] || return 0

    if [ -e /etc/iscsi/iscsi.initramfs ]; then
        log_warning_msg "not deconfiguring network interfaces: iSCSI root is mounted."
        exit 0
    fi

    while read DEV MTPT FSTYPE REST; do
        case $DEV in
        /dev/nbd*|/dev/nd[a-z]*|/dev/etherd/e*|curlftpfs*)
            log_warning_msg "not deconfiguring network interfaces: network devices still mounted."
            exit 0
            ;;
        esac
        case $FSTYPE in
        nfs|nfs4|smbfs|ncp|ncpfs|cifs|coda|ocfs2|gfs|pvfs|pvfs2|fuse.httpfs|fuse.curlftpfs)
            log_warning_msg "not deconfiguring network interfaces: network file systems still mounted."
            exit 0
            ;;
        esac
    done < /proc/mounts
}

check_network_swap() {
    [ -e /proc/swaps ] || return 0

    while read DEV MTPT FSTYPE REST; do
        case $DEV in
        /dev/nbd*|/dev/nd[a-z]*|/dev/etherd/e*)
            log_warning_msg "not deconfiguring network interfaces: network swap still mounted."
            exit 0
            ;;
        esac
    done < /proc/swaps
}

ifup_hotplug () {
    if [ -d /sys/class/net ]
    then
            ifaces=$(for iface in $(ifquery --list --allow=hotplug)
                            do
                                    link=${iface##:*}
                                    link=${link##.*}
                                    if [ -e "/sys/class/net/$link" ]
                                    then
                                        # link detection does not work unless we up the link
                                        ip link set "$iface" up || true
                                        if [ "$(cat /sys/class/net/$link/operstate)" = up ]
                                        then
                                            echo "$iface"
                                        fi
                                    fi
                            done)
            if [ -n "$ifaces" ]
            then
                ifup $ifaces "$@" || true
            fi
    fi
}

set_static network () {

	cat >/etc/network/interfaces <<EOF
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet static
address 192.168.3.111
netmask 255.255.255.0
gateway 192.168.3.1

auto eth1
iface eth1 inet static
address 192.168.1.111
netmask 255.255.255.0
gateway 192.168.1.1
EOF

}

DEF_1G_IP		"192.168.1.111"
DEF_1G_NETWORK 	"255.255.255.0"
DEF_1G_GW 		"192.168.1.1"

DEF_10G_IP		"192.168.3.111"
DEF_10G_NETWORK "255.255.255.0"
DEF_10G_GW 		"192.168.3.1"

#说明
show_usage="args: [-l , -r , -b , -w]\
                                  [--net-1g-ip=, --net-1g-netmask=, --net-1g-gateway=, --net-10g-ip=, --net-10g-netmask=, --net-10g-gateway=]"
								  
#网络参数
net_1g_ip=""
net_1g_netmask=""
net_1g_gateway=""
								  
while [ -n "$1" ]
do
        case "$1" in
                -i|--net-1g-ip) net_1g_ip=$2; shift 2;;
                -n|--net-1g-netmask) net_1g_netmask=$2; shift 2;;
                -g|--net-1g-gateway) net_1g_gateway=$2; shift 2;;
                -o|--net-10g-ip) net_10g_ip=$2; shift 2;;
                -p|--net-10g-netmask) net_10g_netmask=$2; shift 2;;
                -q|--net-10g-gateway) net_10g_gateway=$2; shift 2;;
                --) break ;;
                *) echo $1,$2,$show_usage; break ;;
        esac
done

if [[ -z $net_1g_ip || -z $net_1g_netmask || -z $net_1g_gateway || -z $net_10g_ip ]]; then
        echo $show_usage
        echo "opt_localrepo: $opt_localrepo , opt_url: $opt_url , opt_backupdir: $opt_backupdir , opt_webdir: $opt_webdir"
        exit 0
fi

case "$1" in
set)
		
        set_network $DEF_1G_IP $DEF_1G_NETWORK $DEF_1G_GW $DEF_10G_IP $DEF_10G_NETWORK $DEF_10G_GW 
        ;;

*)
        echo "Usage: /etc/init.d/networking {start|stop|reload|restart|force-reload}"
        exit 1
        ;;
esac

exit 0

# vim: noet ts=8