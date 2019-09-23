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

do_compile() {
	     oe_runmake
}

do_install() {
	     install -d ${D}${bindir}
         install -d ${D}${libdir}
	     install -m 0755 platform ${D}${bindir}
         install -m 0755 shell/platform.sh ${D}${bindir}
         install -m 0755 executor/fft/libfftw3/arm/lib/libfftw3f.so.3 ${D}${libdir}
}
INSANE_SKIP_${PN} = "ldflags"
INSANE_SKIP_${PN}-dev = "ldflags"
