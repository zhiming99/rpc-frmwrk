#!/bin/bash
echo installing development tools...
if ! which sudo; then
    SUDO=""
else
    SUDO="sudo"
fi
${SUDO} dnf -y install which gcc g++ python3 flex bison libtool shtool automake autoconf \
make dbus-devel jsoncpp-devel lz4-devel cmake ccache lz4-libs cppunit \
cppunit-devel dbus dbus-tools dbus-libs jsoncpp bash krb5-devel krb5-libs \
python3-devel python3-pip python3-setuptools \
openssl-devel openssl openssl-libs expect \
fuse3-devel fuse3 fuse3-libs npm webpack vim screen git attr \
swig apache-commons-cli 

${SUDO} dnf -y java-latest-openjdk-devel java-latest-openjdk-headless java-latest-openjdk || ${SUDO} dnf -y java-1.8.0-openjdk-devel java-1.8.0-openjdk-headless java-1.8.0-openjdk   
${SUDO} dnf -y install sip5 || ${SUDO} dnf -y install sip6
${SUDO} dnf -y install python3-wheel python3-numpy rpmdevtools || pip3 install wheel numpy

npm -g install assert browserify buffer exports long lz4 process put safe-buffer \
 stream xxhash xxhashjs minify webpack webpack-cli vm events crypto-browserify \
 stream-browserify

bash ./makerpcf.sh
