#/bin/bash
# This is a script to automate the environment setup, build rpc-frmwrk, config
# rpc-frmwrk and install rpc-frmwrk for Debian and Ubuntu.
if ! which sudo; then
    SUDO=""
else
    SUDO="sudo"
fi
echo installing development tools...
${SUDO} apt-get -y install tzdata
${SUDO} apt-get -y install krb5-user
${SUDO} apt-get install -y gcc g++ python3 python3-dev python3-pip flex bison \
libtool shtool automake autoconf autotools-dev make dbus libdbus-1-3 \
libdbus-1-dev libjsoncpp-dev libkrb5-3 libkrb5-dev liblz4-1 \
liblz4-dev openssl libssl3 libssl-dev libcppunit-1.15-0 libcppunit-dev \
libfuse3-3 libfuse3-dev krb5-user \
bash net-tools procps swig default-jdk-headless cmake libcommons-cli-java ccache

${SUDO} apt-get -y install sip-tools || apt-get -y install sip-dev python3-sip python3-sip-dev || true
${SUDO} apt-get -y install libjsoncpp1 || apt-get -y install libjsoncpp25 || apt-get -y install libjsoncpp24
${SUDO} apt-get -y install git devscripts debhelper expect screen vim npm
${SUDO} apt-get -y install python3-wheel python3-numpy || pip3 install wheel numpy
npm -g install assert browserify buffer exports long lz4 process put safe-buffer stream xxhash xxhashjs minify webpack webpack-cli

bash ./makerpcf.sh
