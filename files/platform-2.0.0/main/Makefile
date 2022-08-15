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
	install -d ${D}/usr/bin
	install -d ${D}/usr/lib
	install -d ${D}/etc/init.d/
	install -d ${D}/var/local/
ifeq ($(CONFIG_SPM_FFT_WINDOWS),y)
	install -d ${D}/etc/win/
endif
ifeq ($(CONFIG_LIBFFTW3),y)
	install -m 0755 ${S}/main/device/rf/adrv/cfg/Rx_BW*.txt ${D}/var/local/
	install -m 0755 ${S}/lib/libfftw3/arm/lib/libfftw3f.so.3 ${D}/usr/lib
endif
	install -m 0755 ${S}/platform ${D}/usr/bin
	install -m 0755 ${S}/main/shell/*.sh ${D}/etc/
ifeq ($(CONFIG_SYSLOG),y)
	install -d ${D}/etc/rsyslog.d
	install -d ${D}/etc/logrotate.d
	install -m 0755 ${S}/main/conf/rsyslog.d/* ${D}/etc/rsyslog.d
	install -m 0755 ${S}/main/conf/logrotate/* ${D}/etc/logrotate.d
endif
	make -C tools $@
	install -m 0755 ${S}/main/shell/install.sh ${D}

ifeq (,$(wildcard ${S}/main/bsp/$(BSP_NAME)/init.d))
	install -m 0755 ${S}/main/shell/init.d/* $(D)/etc/init.d/
else
	install -m 0755 ${S}/main/bsp/$(BSP_NAME)/init.d/* $(D)/etc/init.d/
endif

ifeq (,$(wildcard ${S}/main/bsp/$(BSP_NAME)/etc))
	install -m 0755 ${S}/main/conf/etc/* $(D)/etc/
else
	install -m 0755 ${S}/main/bsp/$(BSP_NAME)/etc/* $(D)/etc/
endif
	$(shell if [ -d ${TOPDIR}/images/linux ]; then cp ${S}/platform ${TOPDIR}/images/linux/; fi)
