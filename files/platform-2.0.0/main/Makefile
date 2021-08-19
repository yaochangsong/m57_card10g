obj-y := main.o
obj-y  					+= bsp/
obj-y  					+= conf/
obj-y 					+= log/
obj-$(CONFIG_DAO)  		+= dao/
obj-y  					+= device/
obj-y  					+= executor/
obj-$(CONFIG_FS)  		+= fs/
obj-y  					+= net/
obj-y  					+= protocol/
obj-y  					+= utils/

install:
	cp ${S}/main/bsp/$(BSP_NAME)/config.json conf/
	install -d ${D}${bindir}
	install -d ${D}${libdir}
	install -d ${D}/etc/init.d/
	install -d ${D}/var/local/
ifeq ($(CONFIG_SPM_FFT_WINDOWS),y)
	install -d ${D}/etc/win/
endif
	install -m 0755 ${S}/platform ${D}${bindir}
	install -m 0755 ${S}/main/shell/platform.sh ${D}/etc/init.d/
	install -m 0755 ${S}/main/shell/led.sh ${D}/etc/
	install -m 0755 ${S}/main/shell/genmac.sh ${D}/etc/
	install -m 0755 ${S}/main/shell/netset.sh ${D}/etc/
	install -m 0755 ${S}/main/shell/check.sh ${D}/etc/
	install -m 0755 ${S}/main/shell/network.sh ${D}/etc/
	install -m 0755 ${S}/main/shell/disk.sh ${D}/etc/
	install -m 0755 ${S}/main/shell/checkproc.sh ${D}/etc/
	install -m 0755 ${S}/main/shell/reset-event.sh ${D}/etc/
	install -m 0755 ${S}/main/shell/reset-button.sh ${D}/etc/
	install -m 0755 ${S}/main/shell/config.sh ${D}/etc/
	#install -m 0755 ${S}/main/shell/sysupgrade_rootfs.sh ${D}/etc/
	install -m 0755 ${S}/main/conf/config.json ${D}/etc/config.json.jffs2
	install -m 0755 ${S}/main/conf/platform_version ${D}/etc/
	install -m 0755 ${S}/main/conf/compile.info ${D}/etc/
	install -m 0755 tools/xjson ${D}${bindir}
	install -m 0755 tools/reset-button  ${D}${bindir}
	$(shell if [ ! -d ${TOPDIR}/images/linux ]; then cp ${S}/platform ${TOPDIR}/images/linux/; fi)
