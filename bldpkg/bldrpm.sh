#!/bin/bash
SRCTREE=$1
PACKAGE_NAME=$2
ARCH=$3
timestamp=`date -R | awk '{print $1,$3,$2,$4}' | sed 's/,//'`
echo $timestamp
sed -i "s:SSSSS:${SRCTREE}:" ${PACKAGE_NAME}.spec
sed -i "s/XXXXX/${timestamp}/" ${PACKAGE_NAME}.spec
sed -i "s:ZZZZZ:\n:" ${PACKAGE_NAME}.spec
cp ${PACKAGE_NAME}.spec ${HOME}/rpmbuild/SPECS
if cd ${HOME}/rpmbuild/SPECS && rpmbuild -ba ${PACKAGE_NAME}.spec; then
    cp ../SRPMS/*.rpm $SRCTREE/bldpkg/
    cp ../RPMS/$ARCH/*.rpm $SRCTREE/bldpkg/
fi

