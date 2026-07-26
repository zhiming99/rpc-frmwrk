#/bin/bash
# This is a script to automate the environment setup, build rpc-frmwrk, config
# rpc-frmwrk and install rpc-frmwrk for Debian and Ubuntu.
if ! command -v sudo > /dev/null; then
    SUDO=""
else
    SUDO="sudo"
fi
echo installing development tools...
${SUDO} apt-get -y install tzdata
${SUDO} apt-get install -y gcc g++ python3 python3-dev python3-pip python3-build flex bison \
libtool shtool automake autoconf autotools-dev make dbus dbus-bin \
libdbus-1-dev libjsoncpp-dev libkrb5-dev \
liblz4-dev openssl libssl-dev libcppunit-dev \
libfuse3-dev fuse3 python3-urwid \
bash net-tools procps swig default-jdk-headless cmake libcommons-cli-java ccache attr gettext

${SUDO} apt-get -y install sip-tools python3-sipbuild || ${SUDO} apt-get -y install sip-dev python3-sip python3-sip-dev || true
${SUDO} apt-get -y install git devscripts debhelper expect screen vim
${SUDO} apt-get -y install python3-wheel python3-numpy || pip3 install wheel numpy

NODE_MAJOR=24
curl -fsSL https://deb.nodesource.com/setup_${NODE_MAJOR}.x | bash - || exit 3
apt-get install -y nodejs

npm -g install assert browserify buffer exports long lz4 process put safe-buffer stream xxhashjs xxhash webpack webpack-cli minify vm events js-sha1 stream-browserify

bash ./makerpcf.sh $@
