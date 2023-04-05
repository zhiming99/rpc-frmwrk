#!/bin/bash
# parameters:
# $1: path to store the keys and certs
# $2: number of client keys, 1 if not specified
# $3: number of server keys, 1 if not specified
# gmssl demo key generator, adapted from GmSSL script
# @https://github.com/guanzhi/GmSSL/blob/master/demos/scripts/tls13demo.sh

PATH=/usr/local/bin:$PATH
if [ "x$1" != "x" -a ! -d $1 ]; then
    echo Usage: bash gmsslkey.sh [directory to store keys] [ number of client keys ] [number of server keys]
    exit 1
fi

if [ "x$1" == "x" ]; then
    targetdir="$HOME/.rpcf/gmssl"
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

pushd $targetdir

if [ ! -f rpcf_serial ]; then
    echo '0' > rpcf_serial
fi

if which gmssl; then
if [ -d private_keys ]; then 
    mv private_keys/* ./
fi

if [ ! -f rootcakey.pem ]; then
    rm *.pem
    gmssl sm2keygen -pass 1234 -out rootcakey.pem
    gmssl certgen -C CN -ST Beijing -L Haidian -O PKU -OU CS -CN ROOTCA -days 3650 -key rootcakey.pem -pass 1234 -out rootcacert.pem -key_usage keyCertSign -key_usage cRLSign -ca
fi

if [ ! -f cakey.pem ]; then
    mkdir backup
    mv rootca*.pem backup/
    rm *.pem
    mv backup/* ./
    rmdir backup
    gmssl sm2keygen -pass 1234 -out cakey.pem
    gmssl reqgen -C CN -ST Beijing -L Haidian -O PKU -OU CS -CN "Sub CA" -key cakey.pem -pass 1234 -out careq.pem
    gmssl reqsign -in careq.pem -days 365 -key_usage keyCertSign -ca -path_len_constraint 0 -cacert rootcacert.pem -key rootcakey.pem -pass 1234 -out cacert.pem
    rm careq.pem
fi

idx_base=`head -n1 rpcf_serial`
let endidx=idx_base+numsvr
for((i=idx_base;i<endidx;i++));do
    gmssl sm2keygen -pass 1234 -out signkey.pem
    gmssl reqgen -C CN -ST Beijing -L Haidian -O PKU -OU CS -CN "server:$i" -key signkey.pem -pass 1234 -out signreq.pem
    gmssl reqsign -in signreq.pem -days 365 -key_usage digitalSignature -cacert cacert.pem -key cakey.pem -pass 1234 -out signcert.pem
    cat signcert.pem > certs.pem
    cat cacert.pem >> certs.pem
    tar zcf serverkeys-$i.tar.gz signkey.pem signcert.pem certs.pem
    rm signreq.pem signkey.pem signcert.pem  certs.pem
done

for((i=0;i<endidx;i++)); do
    if [ -f serverkeys-$i.tar.gz ]; then
        tar zxf serverkeys-$i.tar.gz
        break
    fi
done

let idx_base+=numsvr
let endidx=idx_base+numcli
for((i=idx_base;i<endidx;i++));do
    gmssl sm2keygen -pass 1234 -out clientkey.pem
    gmssl reqgen -C CN -ST Beijing -L Haidian -O PKU -OU CS -CN "client:$i" -key clientkey.pem -pass 1234 -out clientreq.pem
    gmssl reqsign -in clientreq.pem -days 365 -key_usage digitalSignature -cacert cacert.pem -key cakey.pem -pass 1234 -out clientcert.pem
    tar zcf clientkeys-$i.tar.gz clientkey.pem clientcert.pem rootcacert.pem 
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

#gmssl certparse -in clientcert.pem

if [ ! -d ./private_keys ]; then
    mkdir ./private_keys
fi

mv rootcakey.pem cakey.pem private_keys/
mv cacert.pem private_keys/
chmod og-rwx private_keys/rootcakey.pem private_keys/cakey.pem
cp rootcacert.pem private_keys
else
    echo "GmSSL is not installed, and please install GmSSL first."
fi #which gmssl

popd
