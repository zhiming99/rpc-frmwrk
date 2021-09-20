#!/bin/bash
DESTDIR=$1
sed -i 's/x86_64/amd64/g' ./control;
find ${DESTDIR} -name '*.a' -exec rm '{}' ';'
find ${DESTDIR} -perm 0755 | xargs strip
cp ./control ${DESTDIR}/DEBIAN/;
dpkg-deb --build ${DESTDIR} .

