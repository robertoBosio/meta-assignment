DESCRIPTION = "Application to test kernel module ppgmod"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://app.c"
S = "${WORKDIR}"

do_compile() {
    set CFLAGS -g
    ${CC} ${CFLAGS} app.c -lpthread -lm ${LDFLAGS} -o app
    unset CFLAGS
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 app ${D}${bindir}
} 
