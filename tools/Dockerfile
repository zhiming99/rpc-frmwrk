FROM ubuntu
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

RUN apt-get install -y gcc g++ python3 python3-dev python3-pip flex bison \
libtool shtool automake autoconf autotools-dev make dbus libdbus-1-3 \
libdbus-1-dev libjsoncpp1 libjsoncpp-dev libkrb5-3 libkrb5-dev liblz4-1 \
liblz4-dev lz4 openssl libssl1.1 libssl-dev libcppunit-1.15-0 libcppunit-dev \
libfuse3-3 libfuse3-dev krb5-kdc krb5-admin-server krb5-user \
bash net-tools procps swig openjdk-8-jdk-headless;pip3 install numpy wheel
RUN apt-get -y install sip5-tools || apt-get -y install sip-dev python3-sip python3-sip-dev || true
RUN apt-get -y install libjsoncpp1 || apt-get -y install libjsoncpp24
RUN apt-get -y install git;cd ${HOME} && git clone 'https://github.com/zhiming99/rpc-frmwrk.git'
RUN cd ${HOME}/rpc-frmwrk && libtoolize && aclocal && autoreconf -vfi && \
automake --add-missing && autoconf; echo `pwd`;ls -l `pwd`
RUN cd ${HOME}/rpc-frmwrk && ./configure "CPPFLAGS=-O2 -g -DRELEASE -D_DEBUG=0 -fno-strict-aliasing" && make && make install;\
echo 'export LD_LIBRARY_PATH=/usr/local/lib:/usr/local/lib/rpcf'>>${HOME}/.bashrc
ADD ./testcfgs/mincfg.json ${HOME}
RUN python3 /usr/local/bin/rpcf/rpcfgnui.py ${HOME}/mincfg.json
RUN cd ${HOME}/rpc-frmwrk; \
dbus-run-session bash tools/runtest.sh 2 & \
dbus-run-session bash tools/runtest.sh 1

