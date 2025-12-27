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
${SUDO} apt-get install -y gcc g++ python3 python3-dev python3-pip flex bison \
libtool shtool automake autoconf autotools-dev make dbus dbus-bin libdbus-1-3 \
libdbus-1-dev libjsoncpp-dev libkrb5-3 libkrb5-dev liblz4-1 \
liblz4-dev openssl libssl3 libssl-dev libcppunit-1.15-0 libcppunit-dev \
libfuse3-3 libfuse3-dev \
bash net-tools procps swig default-jdk-headless cmake libcommons-cli-java ccache attr

${SUDO} apt-get -y install sip-tools python3-sipbuild || ${SUDO} apt-get -y install sip-dev python3-sip python3-sip-dev || true
for i in 1 4 5 6; do
if ${SUDO} apt-get -y install libjsoncpp$i; then 
    break;
fi
done
if ! dpkg -l libjsoncpp$i > /dev/null; then
    echo Error unable to install libjsoncpp
    exit 1
fi
${SUDO} apt-get -y install git devscripts debhelper expect screen vim npm
${SUDO} apt-get -y install python3-wheel python3-numpy || pip3 install wheel numpy
npm -g install assert browserify buffer exports long lz4 process put safe-buffer \
 stream xxhash xxhashjs minify webpack webpack-cli vm events crypto-browserify \
 stream-browserify

bash ./makerpcf.sh
