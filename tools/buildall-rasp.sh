#/bin/bash
# This is a script to automate the environment setup, build rpc-frmwrk, config
# rpc-frmwrk and install rpc-frmwrk for Debian and Ubuntu.
if ! command -v sudo > /dev/null; then
    SUDO=""
else
    SUDO="sudo"
fi
echo installing development tools...
${SUDO} apt-get -o Acquire::Retries=3 update
${SUDO} apt-get -y install tzdata
${SUDO} apt-get install -y gcc g++ python3 python3-dev python3-pip flex bison \
libtool shtool automake autoconf autotools-dev make dbus dbus-bin \
libdbus-1-dev libjsoncpp-dev libkrb5-dev python3-build \
liblz4-dev openssl libssl-dev libcppunit-dev \
libfuse3-dev bash net-tools procps swig default-jdk-headless cmake \
libcommons-cli-java ccache curl fuse3 python3-urwid gettext

${SUDO} apt-get -y install sip-tools || ${SUDO} apt-get -y install sip-dev python3-sip python3-sip-dev || true

${SUDO} apt-get -y install git devscripts debhelper expect screen vim
${SUDO} apt-get -y install python3-wheel python3-numpy || pip3 install wheel numpy

NODE_MAJOR=18
curl -fsSL https://deb.nodesource.com/setup_${NODE_MAJOR}.x | bash - || exit 3
apt-get install -y nodejs

npm -g install browserify buffer long lz4 process put safe-buffer stream xxhashjs webpack webpack-cli minify events js-sha1 stream-browserify

bash ./makerpcf-rasp.sh $@
