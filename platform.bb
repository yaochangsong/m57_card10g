#
# This file is the platform recipe.
#

SUMMARY = "Simple platform application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://*.c \
	   file://Makefile \
		  "
S = "${WORKDIR}"

inherit update-rc.d
INITSCRIPT_NAME = "platform.sh"
INITSCRIPT_PARAMS = "start 99 5 ."

do_compile() {
	     oe_runmake
}

do_install() {
         install -d ${D}${bindir}
         install -d ${D}${libdir}
         install -d ${D}/etc/init.d/
         install -m 0755 platform ${D}${bindir}
         install -m 0755 shell/platform.sh ${D}/etc/init.d/
         install -m 0755 shell/led.sh ${D}/etc/
		 install -m 0755 shell/network.sh ${D}/etc/
         install -m 0755 shell/check.sh ${D}/etc/
         install -m 0755 shell/disk.sh ${D}/etc/
         install -m 0755 shell/checkproc.sh ${D}/etc/
         install -m 0755 shell/reset-event.sh ${D}/etc/
         install -m 0755 shell/reset-button.sh ${D}/etc/
         install -m 0755 conf/config.json ${D}/etc/
	 install -m 0755 conf/compile.info ${D}/etc/
         install -m 0755 tools/xjson ${D}${bindir}
         install -m 0755 tools/reset-button  ${D}${bindir}
}

INSANE_SKIP_${PN} = "ldflags"
INSANE_SKIP_${PN}-dev = "ldflags"
INSANE_SKIP_${PN} +="already-stripped"
FILES_${PN} += "/usr/*"
