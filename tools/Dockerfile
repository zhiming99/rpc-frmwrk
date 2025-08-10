FROM ubuntu:latest
MAINTAINER woodhead99@gmail.com
RUN apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install tzdata

#install the kerberos servers and client
ADD ./initkrb5.sh /root
RUN apt-get install -qq krb5-kdc krb5-admin-server krb5-user 
#setup the kdc and kadmind and start the servers
CMD bash /root/initkrb5.sh || exit 1
#login with principal zhiming
CMD kinit -k -t /etc/krb5.keytab zhiming || exit 2

RUN apt-get install -y --fix-missing gcc g++ python3 python3-dev python3-pip python3-build flex bison \
libtool shtool automake autoconf autotools-dev make dbus libdbus-1-3 \
libdbus-1-dev libjsoncpp-dev libkrb5-3 libkrb5-dev liblz4-1 \
liblz4-dev lz4 openssl libssl3 libssl-dev libcppunit-1.15-0 libcppunit-dev \
libfuse3-3 libfuse3-dev krb5-kdc krb5-admin-server krb5-user \
bash net-tools procps swig cmake attr \
libcommons-cli-java curl figlet
RUN apt-get install -y openjdk-8-jdk-headless || apt-get install -y \
    openjdk-11-jdk-headless || apt-get install -y openjdk-17-jdk-headless 
# ubuntu 22.04's version of nodejs is outdated, lets install node 20 from nodejs'
# site instead.

#RUN curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.7/install.sh | bash \
# && source .bashrc && nvm install 20 && node -v

RUN apt-get -y install sip-tools python3-sipbuild || apt-get -y install sip-dev python3-sip python3-sip-dev || true
RUN apt-get -y install libjsoncpp1 || apt-get -y install libjsoncpp25 || apt-get -y install libjsoncpp24
RUN apt-get -y install git devscripts debhelper expect vim screen tmux npm iputils-ping;
RUN cd ${HOME} && git clone 'https://github.com/zhiming99/GmSSL.git'
RUN cd ${HOME};cd ./GmSSL;mkdir build;cd build;cmake ..;make;make install
RUN apt-get -y install python3-wheel python3-numpy || pip3 install wheel numpy
RUN npm -g install assert browserify buffer exports long lz4 process put \
 safe-buffer stream xxhash xxhashjs minify webpack webpack-cli vm events crypto-browserify \
 stream-browserify
RUN cd ${HOME};git clone 'https://github.com/zhiming99/rpc-frmwrk.git'
RUN cd ${HOME}/rpc-frmwrk && libtoolize && aclocal && autoreconf -vfi && \
automake --add-missing && autoconf; echo `pwd`;ls -l `pwd`
RUN cd ${HOME}/rpc-frmwrk && bash cfgsel -r && make && make install;\
echo 'export LD_LIBRARY_PATH=/usr/local/lib:/usr/local/lib/rpcf'>>${HOME}/.bashrc
ADD ./testcfgs/mincfg.json /root
RUN python3 /usr/local/bin/rpcf/rpcfgnui.py ${HOME}/mincfg.json
RUN cd ${HOME}/rpc-frmwrk; figlet -cm 60 'RPC-FRMWRK ROCKS !'
# uncomment the following to start the tests
#dbus-run-session bash tools/runtest.sh 2 & \
#dbus-run-session bash tools/runtest.sh 1

