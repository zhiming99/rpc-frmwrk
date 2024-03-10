#!/bin/bash
echo installing development tools...
if ! which sudo; then
    SUDO=""
else
    SUDO="sudo"
fi
${SUDO} dnf install gcc g++ python3 flex bison libtool shtool automake autoconf \
make dbus-devel jsoncpp-devel lz4-devel cmake ccache lz4-libs cppunit \
cppunit-devel dbus dbus-libs jsoncpp bash \
krb5-devel krb5-libs \
python3-devel python3-pip python3-setuptools \
openssl-devel openssl openssl-libs expect \
fuse3-devel fuse3 fuse3-libs npm\
java-1.8.0-openjdk-devel java-1.8.0-openjdk-headless java-1.8.0-openjdk swig apache-commons-cli \
${SUDO} dnf install sip5 || ${SUDO} dnf install sip6
pip3 install wheel numpy || ${SUDO} dnf install python3-wheel python3-numpy
npm install assert browserify buffer exports long lz4 process put safe-buffer stream xxhash webpack

bash ./makerpcf.sh
