#
# This file is the platform recipe.
#

SUMMARY = "Simple platform application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit update-rc.d
INITSCRIPT_NAME = "platform.sh"
INITSCRIPT_PARAMS = "start 99 5 ."

SRC_URI = "file://platform-2.0.0"
S = "${WORKDIR}/platform-2.0.0"

DEPENDS = "${@depend_str('CONFIG_DEPENDS_LIB',d)}"
RDEPENDS_${PN} = "${@depend_str('CONFIG_DEPENDS_LIB',d)}"

EXTRA_OEMAKE = "'CC=${CC}' PETAENV=1 -I${S}. "


def depend_str(b, d):
    import os
    config=d.getVar("S", True)+'/.config'
    if (not os.path.exists(config)):
        #bb.plain("%s is not exist" % (config))
        return ''
    a=''
    with open(config, "r") as data:
            lines = data.readlines()
            for line in lines:
                if b in line:
                    a=eval(line.split("=")[1])
                    #bb.plain("split: %s" % (a))
    return a
    

do_configure () {
	# Specify any needed configure commands here
	bbplain "DEPENDS=${DEPENDS}, RDEPENDS_${PN}=${RDEPENDS_${PN}}"
	:
}


do_menuconfig () {
	make -C ${S}  menuconfig
}

addtask menuconfig

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
