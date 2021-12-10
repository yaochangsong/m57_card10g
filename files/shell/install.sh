#!/bin/sh

INS_GOAHEAD_DIR="/etc/goahead/"
INS_PLATFORM_DIR="/usr/bin/"
INS_MODULE_DIR="/lib/modules/4.4.131-20191204.kylin.desktop-generic/extra/"

install_gohead()
{
	
	echo "install goahead in ${INS_GOAHEAD_DIR}..."
	if [ ! -d "${INS_GOAHEAD_DIR}" ]; then
		mkdir ${INS_GOAHEAD_DIR}
	fi
	
	install -m 0755 goahead/goahead.sh /etc/init.d/
	install -m 0655 goahead/bin/libgo.so ${INS_GOAHEAD_DIR}
	install -m 0655 goahead/bin/*.key ${INS_GOAHEAD_DIR}
	install -m 0655 goahead/bin/*.txt ${INS_GOAHEAD_DIR}
	install -m 0655 goahead/bin/*.crt ${INS_GOAHEAD_DIR}
	install -m 0755 goahead/bin/goahead ${INS_GOAHEAD_DIR}
	cp -rf goahead/bin/web ${INS_GOAHEAD_DIR}
}

install_platform()
{
	echo "install platform!"
	install -m 0755 platform/platform.sh /etc/init.d/
	install -m 0755 platform/nr_config.json /etc/
	install -m 0755 platform/sroute ${INS_PLATFORM_DIR}
	install -m 0755 platform/platform ${INS_PLATFORM_DIR}
	install -m 0755 platform/xdma.ko ${INS_MODULE_DIR}
	
}

install_gohead
install_platform
