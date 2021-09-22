#!/bin/bash
DESTDIR=$1
prefix=$2
pkg=$3

pkgfile=$(basename $pkg)
pkgname=${pkgfile%%-*}

if ! which dpkg-deb; then
    exit 0
fi

if [ ! -e ${DESTDIR}/DEBIAN ]; then mkdir ${DESTDIR}/DEBIAN; fi
cat << EOF >> ${DESTDIR}/DEBIAN/postinst
pip3 install ${prefix}/lib/${pkgname}/${pkgfile};
EOF

cat << EOF >> ${DESTDIR}/DEBIAN/postrm
pip3 uninstall -y ${pkgname}
EOF

chmod 0755 ${DESTDIR}/DEBIAN/postinst
chmod 0755 ${DESTDIR}/DEBIAN/postrm
