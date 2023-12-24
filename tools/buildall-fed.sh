#!/bin/bash
echo installing development tools...
sudo dnf install gcc g++ python3 flex bison libtool shtool automake autoconf \
make dbus-devel jsoncpp-devel lz4-devel cmake ccache lz4-libs cppunit \
cppunit-devel dbus dbus-libs jsoncpp bash \
krb5-devel krb5-libs \
python3-devel python3-pip python3-setuptools \
openssl-devel openssl openssl-libs expect \
fuse3-devel fuse3 fuse3-libs \
java-1.8.0-openjdk-devel java-1.8.0-openjdk-headless java-1.8.0-openjdk swig apache-commons-cli \
sudo dnf install sip5 || sudo dnf install sip6
pip3 install wheel numpy || sudo dnf install python3-wheel python3-numpy

bash ./makerpcf.sh
