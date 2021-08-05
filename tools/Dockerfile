FROM ubuntu
MAINTAINER woodhead99@gmail.com
RUN apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install tzdata
RUN apt-get install -y gcc g++ python3 python3-dev flex bison libtool shtool automake autoconf autotools-dev make dbus libdbus-1-3 libdbus-1-dev libjsoncpp1 libjsoncpp-dev libkrb5-3 libkrb5-dev liblz4-1 liblz4-dev lz4 openssl libssl1.1 libssl-dev libcppunit-1.15-0 libcppunit-dev bash net-tools
RUN apt-get -y install sip5-tools || apt-get -y install sip-dev python3-sip python3-sip-dev || true
ADD rpcf-0.1.0.tar.gz /root/
RUN cd /root/rpcf-0.1.0;/bin/sh /root/rpcf-0.1.0/configure;make;make install;echo 'export LD_LIBRARY_PATH=/usr/local/lib:/usr/local/lib/rpcf'>>/root/.bashrc
RUN dbus-run-session bash /root/rpcf-0.1.0/tools/runtest.sh 2; dbus-run-session /root/rpcf-0.1.0/tools/runtest.sh 1
