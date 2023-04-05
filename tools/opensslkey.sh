#!/bin/bash
# parameters:
# $1: path to store the keys and certs, ~/.rpcf/openssl if not specified
# $2: number of client keys, 1 if not specified
# $3: number of server keys, 1 if not specified
# openssl demo key generator

if [ "x$1" != "x" -a ! -d $1 ]; then
    echo Usage: bash opensslkey.sh [directory to store keys] [ number of client keys ] [number of server keys]
    exit 1
fi

if [ "x$1" == "x" ]; then
    targetdir="$HOME/.rpcf/openssl"
    if [ ! -d $targetdir ]; then
        mkdir -p $targetdir
    fi
else
    targetdir=$1
fi

if [ "x$2" == "x" ];then
    numcli=1
else
    numcli=$2
fi

if [ "x$3" == "x" ];then
    numsvr=1
else
    numsvr=$3
fi

SSLCNF_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)
echo $SSLCNF_DIR
SSLCNF=${SSLCNF_DIR}/testcfgs/openssl.cnf

if [ ! -d $targetdir ]; then
    exit 1
fi

pushd $targetdir

if [ ! -f rpcf_serial ]; then
    echo '0' > rpcf_serial
fi

if which openssl; then

if [ -d private_keys ]; then 
    mv private_keys/* ./
fi

if [ ! -d ./demoCA/newcerts ]; then
    mkdir -p ./demoCA/newcerts;
    mkdir -p ./demoCA/crl;

    if [ ! -f ./demoCA/index.txt ]; then
        touch ./demoCA/index.txt
    fi

    if [ ! -f ./demoCA/serial ]; then
        echo '01' > ./demoCA/serial
    fi
fi

if [ ! -f rootcakey.pem ]; then
    rm *.pem
    openssl genrsa -out rootcakey.pem 4096
    openssl req -new -sha256 -x509 -days 3650 -config ${SSLCNF} -extensions v3_ca -key rootcakey.pem -out rootcacert.pem -subj "/C=CN/ST=Shaanxi/L=Xian/O=Yanta/OU=rpcf/CN=ROOTCA/emailAddress=woodhead99@gmail.com"
fi

if [ ! -f cakey.pem ]; then
    mkdir backup
    mv rootca*.pem backup/
    rm *.pem
    mv backup/* ./
    rmdir backup

    openssl genrsa -out cakey.pem 2048
    openssl req -new -sha256 -key cakey.pem  -out careq.pem -days 365 -subj "/C=CN/ST=Shaanxi/L=Xian/O=Yanta/OU=rpcf/CN=Sub CA/emailAddress=woodhead99@gmail.com"
    openssl ca -days 365 -cert rootcacert.pem -keyfile rootcakey.pem -md sha256 -extensions v3_ca -config ${SSLCNF} -in careq.pem -out cacert.pem
    rm careq.pem
    cat cacert.pem > certs.pem
    cat rootcacert.pem >> certs.pem
fi

idx_base=`cat rpcf_serial`
let endidx=idx_base+numsvr
for((i=idx_base;i<endidx;i++));do
    openssl genrsa -out signkey.pem 2048
    openssl req -new -sha256 -key signkey.pem -out signreq.pem -extensions usr_cert -config ${SSLCNF} -subj "/C=CN/ST=Shaanxi/L=Xian/O=Yanta/OU=rpcf/CN=Server:$i"
    if which expect; then
        openssl ca -days 365 -cert cacert.pem -keyfile cakey.pem -md sha256 -extensions usr_cert -config ${SSLCNF} -in signreq.pem -out signcert.pem
    else
        openssl x509 -req -in signreq.pem -CA cacert.pem -CAkey cakey.pem -days 365 -out signcert.pem -CAcreateserial
    fi
    tar zcf serverkeys-$idx_base.tar.gz signkey.pem signcert.pem certs.pem
    rm signreq.pem signkey.pem signcert.pem
done

#keep the first server keys in the directory
for((i=0;i<endidx;i++)); do
    if [ -f serverkeys-$i.tar.gz ]; then
        tar zxf serverkeys-$i.tar.gz
        break
    fi
done

let idx_base+=numsvr
let endidx=idx_base+numcli
for((i=idx_base;i<endidx;i++));do
    openssl genrsa -out clientkey.pem 2048
    openssl req -new -sha256 -key clientkey.pem -out clientreq.pem -extensions usr_cert -config ${SSLCNF} -subj "/C=CN/ST=Shaanxi/L=Xian/O=Yanta/OU=rpcf/CN=client:$i"
    if which expect; then
        openssl ca -days 365 -cert cacert.pem -keyfile cakey.pem -md sha256 -extensions usr_cert -config ${SSLCNF} -in clientreq.pem -out clientcert.pem
    else
        openssl x509 -req -in clientreq.pem -CA cacert.pem -CAkey cakey.pem -days 365 -out clientcert.pem -CAcreateserial
    fi
    tar zcf clientkeys-$i.tar.gz clientkey.pem clientcert.pem certs.pem
    rm clientkey.pem clientreq.pem clientcert.pem
done

#keep the first client keys in the directory
for((i=0;i<endidx;i++)); do
    if [ -f clientkeys-$i.tar.gz ]; then
        tar zxf clientkeys-$i.tar.gz
        break
    fi
done

let idx_base+=numcli
echo $idx_base > rpcf_serial
 
if [ ! -d ./private_keys ]; then
    mkdir ./private_keys
fi

mv rootcakey.pem rootcacert.pem cakey.pem cacert.pem private_keys/
chmod og-rwx private_keys/rootcakey.pem private_keys/cakey.pem
cp certs.pem private_keys

else
    echo openssl is not installed, and please install openssl first.
fi #which openssl
popd
