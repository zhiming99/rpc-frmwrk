#!/bin/sh
# gmssl demo key generator, adapted from GmSSL script from 
# https://github.com/guanzhi/GmSSL/blob/master/demos/scripts/tls13demo.sh
PATH=/usr/local/bin:$PATH
targetdir=$1
if [ ! -d $targetdir ]; then
    exit 1
fi

cd $targetdir

if which gmssl; then

gmssl sm2keygen -pass 1234 -out rootcakey.pem
gmssl certgen -C CN -ST Beijing -L Haidian -O PKU -OU CS -CN ROOTCA -days 3650 -key rootcakey.pem -pass 1234 -out rootcacert.pem -key_usage keyCertSign -key_usage cRLSign -ca

gmssl sm2keygen -pass 1234 -out cakey.pem
gmssl reqgen -C CN -ST Beijing -L Haidian -O PKU -OU CS -CN "Sub CA" -key cakey.pem -pass 1234 -out careq.pem
gmssl reqsign -in careq.pem -days 365 -key_usage keyCertSign -ca -path_len_constraint 0 -cacert rootcacert.pem -key rootcakey.pem -pass 1234 -out cacert.pem

gmssl sm2keygen -pass 1234 -out signkey.pem
gmssl reqgen -C CN -ST Beijing -L Haidian -O PKU -OU CS -CN localhost -key signkey.pem -pass 1234 -out signreq.pem
gmssl reqsign -in signreq.pem -days 365 -key_usage digitalSignature -cacert cacert.pem -key cakey.pem -pass 1234 -out signcert.pem

cat signcert.pem > certs.pem
cat cacert.pem >> certs.pem

gmssl sm2keygen -pass 1234 -out clientkey.pem
gmssl reqgen -C CN -ST Beijing -L Haidian -O PKU -OU CS -CN Client -key clientkey.pem -pass 1234 -out clientreq.pem
gmssl reqsign -in clientreq.pem -days 365 -key_usage digitalSignature -cacert cacert.pem -key cakey.pem -pass 1234 -out clientcert.pem
#gmssl certparse -in clientcert.pem

rm *req.pem
rm signcert.pem
if [ ! -d ./keys ]; then
    mkdir ./keys
fi
mv rootcakey.pem cakey.pem keys/
chmod og-rwx keys/*
chmod og-rwx signkey.pem clientkey.pem

fi
