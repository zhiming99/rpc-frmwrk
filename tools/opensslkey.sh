#!/bin/bash
# parameters:
# $1: path to store the keys and certs, ~/.rpcf/openssl if not specified
# $2: number of client keys, 1 if not specified
# $3: number of server keys, 1 if not specified
# openssl demo key generator

bitwidth=2048
if [ "x$1" != "x" -a ! -d $1 ]; then
    echo Usage: bash opensslkey.sh [directory to store keys] [ number of client keys ] [number of server keys]
    exit 1
fi

if [ "x$1" == "x" ]; then
    targetdir="$HOME/.rpcf/openssl"
    if [ ! -d $targetdir ]; then
        mkdir -p $targetdir
        chmod 700 $targetdir
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

RPCF_BIN_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)
if [ "x$(basename ${RPCF_BIN_DIR} )" == "xtools" ]; then
    SSLCNF=${RPCF_BIN_DIR}/testcfgs/openssl.cnf
elif [ "x$(basename ${RPCF_BIN_DIR} )" == "xrpcf" ]; then
    SSLCNF="/etc/rpcf/openssl.cnf"
    if [ ! -f $SSLCNF ]; then
        SSLCNF="${RPCF_BIN_DIR}/../../etc/rpcf/openssl.cnf"
        if [ ! -f $SSLCNF ]; then
            echo cannot find openssl.cnf
            exit 1
        fi
    fi
fi

if [ ! -d $targetdir ]; then
    exit 1
fi

pushd $targetdir

if [ ! -f rpcf_serial ]; then
    echo '0' > rpcf_serial
    if ((numsvr==0)); then
        numsvr=1
    fi
    if ((numcli==0));then
        numcli=1
    fi
fi

if which openssl; then

if [ -d private_keys ]; then 
    mv -f private_keys/* ./
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
    rm -rf *.pem
    openssl genrsa -out rootcakey.pem 4096
    openssl req -new -sha256 -x509 -days 3650 -config ${SSLCNF} -extensions v3_ca -key rootcakey.pem -out rootcacert.pem -subj "/C=CN/ST=Shaanxi/L=Xian/O=Yanta/OU=rpcf/CN=ROOTCA/emailAddress=woodhead99@gmail.com"
fi

echo generating certs.pem
if [ ! -f cakey.pem ]; then
    mkdir backup
    mv -f rootca*.pem backup/
    rm -rf *.pem
    mv -f backup/* ./
    rmdir backup

    openssl genrsa -out cakey.pem ${bitwidth}
    openssl req -new -sha256 -key cakey.pem  -out careq.pem -subj "/C=CN/ST=Shaanxi/L=Xian/O=Yanta/OU=rpcf/CN=Sub CA/emailAddress=woodhead99@gmail.com"
    openssl ca -days 1095 -cert rootcacert.pem -keyfile rootcakey.pem -md sha256 -extensions v3_ca -config ${SSLCNF} -in careq.pem -out cacert.pem
    rm careq.pem
    cat cacert.pem > certs.pem
    cat rootcacert.pem >> certs.pem
fi

idx_base=`head -n1 rpcf_serial`
if ((idx_base < 0 )); then
    exit 1
fi

echo generating server keys
let endidx=idx_base+numsvr
for((i=idx_base;i<endidx;i++));do
    chmod 600 signcert.pem signkey.pem > /dev/null 2>&1 || true
    openssl genrsa -out signkey.pem ${bitwidth}
    openssl req -new -sha256 -key signkey.pem -out signreq.pem -extensions usr_cert -config ${SSLCNF} -subj "/C=CN/ST=Shaanxi/L=Xian/O=Yanta/OU=rpcf/CN=Server-$i" -addext "subjectAltName=DNS:Server$i"
    if which expect; then
        openssl ca -days 365 -cert cacert.pem -keyfile cakey.pem -md sha256 -extensions usr_cert -config ${SSLCNF} -in signreq.pem -out signcert.pem
    else
        openssl x509 -req -in signreq.pem -CA cacert.pem -CAkey cakey.pem -days 365 -out signcert.pem -CAcreateserial
    fi
    tar zcf serverkeys-$i.tar.gz signkey.pem signcert.pem certs.pem
    rm signreq.pem signkey.pem signcert.pem
done

function find_key_to_show()
{
    if (($1==1)); then
        fn='serverkeys'
    else
        fn='clientkeys'
    fi
    
    startkey=0
    for((i=endidx;i>=idx_base;i--)); do
        if [ -f $fn-$i.tar.gz ]; then
            startkey=$i
        fi
    done
    if ((startkey >= idx_base)); then
        tar zxf $fn-$startkey.tar.gz
    else
        if ((idx_base==0)); then
            exit 1
        fi
        for ((i=idx_base-1;i>=0;i--)); do
            if [ -f $fn-$i.tar.gz ];then
                tar zxf $fn-$i.tar.gz
                startkey=$i
                break
            fi
        done
    fi
}

echo generating client keys
#keep the first generated server keys as the current server keys in the directory
startkey=0
find_key_to_show 1
echo $startkey > svridx
svr_idx=$startkey

let idx_base+=numsvr
let endidx=idx_base+numcli
for((i=idx_base;i<endidx;i++));do
    chmod 600 clientcert.pem clientkey.pem > /dev/null 2>&1 || true
    openssl genrsa -out clientkey.pem ${bitwidth}
    openssl req -new -sha256 -key clientkey.pem -out clientreq.pem -extensions usr_cert -config ${SSLCNF} -subj "/C=CN/ST=Shaanxi/L=Xian/O=Yanta/OU=rpcf/CN=Client-$i"
    if which expect; then
        openssl ca -days 365 -cert cacert.pem -keyfile cakey.pem -md sha256 -extensions usr_cert -config ${SSLCNF} -in clientreq.pem -out clientcert.pem
    else
        openssl x509 -req -in clientreq.pem -CA cacert.pem -CAkey cakey.pem -days 365 -out clientcert.pem -CAcreateserial
    fi
    tar zcf clientkeys-$i.tar.gz clientkey.pem clientcert.pem certs.pem
    rm clientkey.pem clientreq.pem clientcert.pem
done

#keep the first generated client keys as the current client keys in the directory
find_key_to_show 0
echo $startkey > clidx
cli_idx=$startkey

mv -f rpcf_serial rpcf_serial.old
echo $endidx > rpcf_serial

if [ ! -d ./private_keys ]; then
    mkdir ./private_keys
fi

mv -f rootcakey.pem rootcacert.pem cakey.pem cacert.pem private_keys/
chmod 400 private_keys/rootcakey.pem private_keys/cakey.pem
cp certs.pem private_keys

svr_end=$idx_base
let idx_base-=numsvr

if (( svr_idx >= idx_base )); then
    if ! tar cf instsvr.tar serverkeys-$svr_idx.tar.gz; then
        exit 1
    fi
    for ((i=svr_idx+1;i<svr_end;i++)); do
        if [ -f serverkeys-$i.tar.gz ]; then
            tar rf instsvr.tar serverkeys-$i.tar.gz
        fi
    done

    tar rf instsvr.tar svridx
    mv -f clidx endidx
    tar rf instsvr.tar endidx
    mv -f endidx clidx
fi 

if (( cli_idx >= svr_end )); then
    if ! tar cf instcli.tar clientkeys-$cli_idx.tar.gz; then
        exit 1
    fi
    for ((i=cli_idx+1;i<endidx;i++)); do
        if [ -f clientkeys-$i.tar.gz ]; then
            tar rf instcli.tar clientkeys-$i.tar.gz
        fi
    done
    tar rf instcli.tar clidx
    mv -f rpcf_serial endidx
    tar rf instcli.tar endidx
    mv -f endidx rpcf_serial
fi 

else
    echo openssl is not installed, and please install openssl first.
fi #which openssl
popd
