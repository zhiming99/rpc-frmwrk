#!/bin/bash
if [ -z "$1" ] || ( [ "$1" != "DEB" ] && [ "$1" != "RPM" ] ); then
    echo "usage: $0 <DEB|RPM>"
    echo "  DEB to build debian package, RPM to build rpm package."
    exit 1
fi

echo "haha `pwd`"

pkgtype="$1"
if [ ! -d build ]; then
    if ! mkdir build; then
        echo "Cannot create 'build' directory"
        exit 1
    fi
fi
shift
pushd build
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_SYSCONFDIR=/etc $@ ..
make -j4

cpack -G $pkgtype

# restore the cache to /usr/local
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
popd
