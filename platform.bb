#
# This file is the platform recipe.
#

SUMMARY = "Simple platform application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

#SRC_URI = "file://*.c \
#	   file://Makefile \
#		  "
#S = "${WORKDIR}"

inherit update-rc.d
INITSCRIPT_NAME = "platform.sh"
INITSCRIPT_PARAMS = "start 99 5 ."

DEPENDS = " libiio"
RDEPENDS_${PN} += " libiio"
SRC_URI = "file://platform-2.0.0"
S = "${WORKDIR}/platform-2.0.0"
CFLAGS_prepend = "-I ${S}/include"

EXTRA_OEMAKE = "'CC=${CC}' PETAENV=1 -I${S}. "

do_configure () {
	# Specify any needed configure commands here
	:
}

do_compile () {
	# You will almost certainly need to add additional arguments here
	oe_runmake S=${S}
}

do_install () {
	# NOTE: unable to determine what to put here - there is a Makefile but no
	# target named "install", so you will need to define this yourself
	oe_runmake install  D=${D} S=${S} TOPDIR=${TOPDIR}/.. BINDIR=${bindir} SBINDIR=${sbindir} \
						MANDIR=${mandir} INCLUDEDIR=${includedir} 
}
INSANE_SKIP_${PN} = "ldflags"
INSANE_SKIP_${PN}-dev = "ldflags"
INSANE_SKIP_${PN} +="already-stripped"
FILES_${PN} += "/usr/*"
