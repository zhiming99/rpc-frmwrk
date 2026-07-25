FROM ubuntu:24.04 AS stage0
MAINTAINER woodhead99@gmail.com
RUN apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install tzdata locales apt-utils gnupg
SHELL ["/bin/bash", "-c"]

RUN sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && locale-gen
ENV LANG=en_US.UTF-8 LANGUAGE=en_US:en LC_ALL=en_US.UTF-8

#install the kerberos servers and client
ADD ./initkrb5.sh /root
RUN apt-get install -qq krb5-kdc krb5-admin-server krb5-user
#setup the kdc and kadmind and start the servers
CMD bash /root/initkrb5.sh || exit 1
#login with principal zhiming
CMD kinit -k -t /etc/krb5.keytab zhiming || exit 2

RUN apt-get install -y --fix-missing gcc g++ python3 python3-dev python3-pip python3-build flex bison \
libtool shtool automake autoconf autotools-dev make dbus dbus-bin python3-venv \
libdbus-1-dev libjsoncpp-dev libkrb5-dev \
liblz4-dev lz4 openssl libssl-dev libcppunit-dev \
libfuse3-dev fuse3 bash net-tools procps swig cmake attr \
libcommons-cli-java curl figlet gettext \
git devscripts debhelper expect vim screen tmux iputils-ping

RUN apt-get install -y default-jdk-headless || apt-get install -y openjdk-8-jdk-headless
RUN apt-get install -y sip-tools python3-sipbuild || apt-get install -y sip-dev python3-sip python3-sip-dev || true

# Create our clean sandbox environment 
RUN python3 -m venv /opt/venv

# Inject the environment paths globally for all future layers and execution tasks
ENV PATH="/opt/venv/bin:$PATH"

RUN pip install --upgrade pip && pip install sip wheel numpy urwid

# ubuntu 22.04's official version of nodejs is outdated, lets install node 20 from nodejs'
# site instead.

ARG NODE_MAJOR=24

RUN curl -fsSL https://deb.nodesource.com/setup_${NODE_MAJOR}.x | bash - || exit 3

# 3. Install Node.js directly via apt
RUN apt-get install -y nodejs || touch ${HOME}/nojs

RUN cd ${HOME};for i in $(seq 1 100);do if git clone 'https://github.com/zhiming99/GmSSL.git'; then break; fi;done
RUN cd ${HOME};cd ./GmSSL;mkdir build;cd build;cmake ..;make;make install
RUN npm -g install assert browserify buffer exports long lz4 process put safe-buffer stream xxhashjs xxhash webpack webpack-cli minify vm events js-sha1 stream-browserify
RUN cd ${HOME};for i in $(seq 1 100); do if git clone 'https://github.com/zhiming99/rpc-frmwrk.git';then break;fi;done
RUN cd ${HOME}/rpc-frmwrk && libtoolize && aclocal && autoreconf -vfi && \
automake --add-missing && autoconf; echo `pwd`;ls -l `pwd`

RUN if [ -f ${HOME}/nojs ]; then opt='--disable-js'; fi; \
    cd ${HOME}/rpc-frmwrk && bash cfgsel -r $opt && make -j4 && make install;

ENV LD_LIBRARY_PATH="/usr/local/lib:/usr/local/lib/rpcf:${LD_LIBRARY_PATH}"

ADD ./testcfgs/mincfg.json /root
RUN python3 /usr/local/bin/rpcf/rpcfgnui.py ${HOME}/mincfg.json
RUN if [ -f ${HOME}/nojs ]; then echo -e "\e[33mrpc-frmwrk is built without JS support \e[0m"; rm -f ${HOME}/nojs; else cd ${HOME}/rpc-frmwrk; figlet -cm 60 'RPC-FRMWRK ROCKS !';fi

# uncomment the following to start the tests
#dbus-run-session bash tools/runtest.sh 2 & \
#dbus-run-session bash tools/runtest.sh 1

