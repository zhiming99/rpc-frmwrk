#!/bin/bash
# parameters:
# $1: path to store the keys and certs
# $2: number of client keys
# $3: number of server keys
# gmssl demo key generator, adapted from GmSSL script from 
# https://github.com/guanzhi/GmSSL/blob/master/demos/scripts/tls13demo.sh
PATH=/usr/local/bin:$PATH
targetdir=$1
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

if [ ! -d $targetdir ]; then
    exit 1
fi

pushd $targetdir

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
fi

for((i=1;i<numsvr;i++));do
gmssl sm2keygen -pass 1234 -out signkey.pem
gmssl reqgen -C CN -ST Beijing -L Haidian -O PKU -OU CS -CN localhost -key signkey.pem -pass 1234 -out signreq.pem
gmssl reqsign -in signreq.pem -days 365 -key_usage digitalSignature -cacert cacert.pem -key cakey.pem -pass 1234 -out signcert.pem
cat signcert.pem > certs.pem
cat cacert.pem >> certs.pem
tar zcf serverkeys-$i.tar.gz signkey.pem signcert.pem certs.pem
rm signreq.pem signkey.pem signcert.pem  certs.pem
done

if [ ! -f signkey.pem ]; then
gmssl sm2keygen -pass 1234 -out signkey.pem
gmssl reqgen -C CN -ST Beijing -L Haidian -O PKU -OU CS -CN localhost -key signkey.pem -pass 1234 -out signreq.pem
gmssl reqsign -in signreq.pem -days 365 -key_usage digitalSignature -cacert cacert.pem -key cakey.pem -pass 1234 -out signcert.pem
cat signcert.pem > certs.pem
cat cacert.pem >> certs.pem
fi

for((i=0;i<numcli;i++));do
gmssl sm2keygen -pass 1234 -out clientkey.pem
gmssl reqgen -C CN -ST Beijing -L Haidian -O PKU -OU CS -CN Client -key clientkey.pem -pass 1234 -out clientreq.pem
gmssl reqsign -in clientreq.pem -days 365 -key_usage digitalSignature -cacert cacert.pem -key cakey.pem -pass 1234 -out clientcert.pem
tar zcf clientkeys-$i.tar.gz clientkey.pem clientreq.pem clientcert.pem
rm clientkey.pem clientreq.pem clientcert.pem
done

#gmssl certparse -in clientcert.pem

if [ ! -d ./private_keys ]; then
    mkdir ./private_keys
fi

mv rootcakey.pem rootcacert.pem cakey.pem private_keys/
chmod og-rwx private_keys/rootcakey.pem private_keys/cakey.pem
chmod og-rwx signkey.pem

fi #which gmssl
