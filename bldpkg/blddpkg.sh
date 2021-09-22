#!/bin/bash
DESTDIR=$1
pkgname=$2
curdir=`pwd`

sed -i 's/x86_64/amd64/g' ./control;
if [ ! -d ./stripped/lib ]; then
    mkdir -p ./stripped/lib/;
else
    rm -rf ./stripped/*;
    mkdir -p ./stripped/lib/;
fi
rm ${pkgname}*.deb
find ${DESTDIR} -name '*.a' -exec mv '{}' ./stripped/lib/ ';' 
stripList=`find ${DESTDIR} -perm 0755 | xargs file | grep ELF | awk -F ':' '{print $1}'`
for i in $stripList; do bash ./stripsym.sh $i ${curdir}/stripped/dbgsyms;done

#fix the broken dependency of libjsoncpp
os_name=`cat /etc/os-release | grep '^ID=' | awk -F '=' '{print $2}'`
if [ "${os_name}" != "ubuntu" ]; then
    exit 0
fi

ubuntu_ver=`cat /etc/os-release | grep VERSION_ID | awk -F '=' '{print $2}'`
if [[ "$ubuntu_ver" > "21.00" ]]; then
    sed -i 's/libjsoncpp1/libjsoncpp24/' ./control
    ubuntu_ver="u21"
else
    ubuntu_ver="u20"
fi


cp ./control ${DESTDIR}/DEBIAN/;
dpkg-deb --build ${DESTDIR} .
mv ${pkgname}.deb ${pkgname}_${ubuntu_ver}.deb
echo building debug symbol package...
tar jcf ${pkgname}_${ubuntu_ver}.dbgsym.tar.bz2 ./stripped
rm -rf ./stripped

