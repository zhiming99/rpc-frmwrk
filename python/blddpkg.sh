#!/bin/bash
DISTDIR=$1
DEBDIR=$1/debian
libdir=$2
pkg=$3

pkgfile=$(basename $pkg)
pkgname=${pkgfile%%-*}

if ! which debuild; then
    exit 0
fi

if [ ! -e ${DEBDIR} ]; then exit 2; fi

echo sed -i "s:XXXX:${libdir}:" ${DEBDIR}/postinst
sed -i "s:XXXX:${libdir}:" ${DEBDIR}/postinst
cat ${DEBDIR}/postinst
