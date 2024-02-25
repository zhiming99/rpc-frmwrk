#/bin/bash
# This is a script to automate the environment setup, build rpc-frmwrk, config
# rpc-frmwrk and install rpc-frmwrk for Debian and Ubuntu.
echo installing development tools...
sudo apt-get -y install tzdata
sudo apt-get install krb5-kdc krb5-admin-server krb5-user
sudo apt-get install -y gcc g++ python3 python3-dev python3-pip flex bison \
libtool shtool automake autoconf autotools-dev make dbus libdbus-1-3 \
libdbus-1-dev libjsoncpp-dev libkrb5-3 libkrb5-dev liblz4-1 \
liblz4-dev lz4 openssl libssl3 libssl-dev libcppunit-1.15-0 libcppunit-dev \
libfuse3-3 libfuse3-dev krb5-kdc krb5-admin-server krb5-user \
bash net-tools procps swig default-jdk-headless cmake libcommons-cli-java ccache

sudo apt-get -y install sip-tools || apt-get -y install sip-dev python3-sip python3-sip-dev || true
sudo apt-get -y install libjsoncpp1 || apt-get -y install libjsoncpp25 || apt-get -y install libjsoncpp24
sudo apt-get -y install git devscripts expect
pip3 install wheel numpy || sudo apt-get -y install python3-wheel python3-numpy

bash ./makerpcf.sh
