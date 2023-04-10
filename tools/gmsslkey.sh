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
    if ((numsvr==0)); then
        numsvr=1
    fi
    if ((numcli==0));then
        numcli=1
    fi
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
if ((idx_base < 0 )); then
    exit 1
fi

let endidx=idx_base+numsvr
for((i=idx_base;i<endidx;i++));do
    gmssl sm2keygen -pass 1234 -out signkey.pem
    gmssl reqgen -C CN -ST Beijing -L Haidian -O PKU -OU CS -CN "server:$i" -key signkey.pem -pass 1234 -out signreq.pem
    gmssl reqsign -in signreq.pem -days 365 -key_usage digitalSignature -cacert cacert.pem -key cakey.pem -pass 1234 -out signcert.pem
    cat signcert.pem > certs.pem
    cat cacert.pem >> certs.pem
    tar zcf serverkeys-$i.tar.gz signkey.pem cacert.pem certs.pem
    rm signreq.pem signkey.pem signcert.pem  certs.pem
done

function find_key_to_show()
{
    if (($1==1 )); then
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

#keep the generated first server keys in the directory
startkey=0
find_key_to_show 1
echo $startkey > svridx
svr_idx=$startkey

let idx_base+=numsvr
let endidx=idx_base+numcli
for((i=idx_base;i<endidx;i++));do
    gmssl sm2keygen -pass 1234 -out clientkey.pem
    gmssl reqgen -C CN -ST Beijing -L Haidian -O PKU -OU CS -CN "client:$i" -key clientkey.pem -pass 1234 -out clientreq.pem
    gmssl reqsign -in clientreq.pem -days 365 -key_usage digitalSignature -cacert cacert.pem -key cakey.pem -pass 1234 -out clientcert.pem
    tar zcf clientkeys-$i.tar.gz clientkey.pem clientcert.pem rootcacert.pem 
    rm clientkey.pem clientreq.pem clientcert.pem
done

#keep the first generated client keys in the directory
find_key_to_show 0
echo $startkey > clidx
cli_idx=$startkey

mv rpcf_serial rpcf_serial.old
echo $endidx > rpcf_serial

#gmssl certparse -in clientcert.pem

if [ ! -d ./private_keys ]; then
    mkdir ./private_keys
fi

mv rootcakey.pem cakey.pem private_keys/
chmod 400 private_keys/rootcakey.pem private_keys/cakey.pem
cp cacert.pem rootcacert.pem private_keys/

svr_end=$idx_base
let idx_base-=numsvr

cat > instcfg.sh << EOF
#!/bin/bash
if [ "x\$1" == "x" ]; then
    echo "Usage: \$0 <key idx starting from zero>"
    exit 1
fi
if [ -f USESSL ]; then
    keydir=\$HOME/.rpcf/gmssl
    if [ ! -d \$keydir ]; then
        mkdir -p \$keydir || exit 1
    fi
    updinitcfg=/usr/bin/rpcf/updinitcfg.py
    if [ ! -f \$updinitcfg ]; then
        updinitcfg=/usr/local/bin/rpcf/updinitcfg.py
        if [ ! -f \$updinitcfg ];then
            exit 1
        fi
    fi
    if [ -f clidx ]; then
        idx_base=\$(head -n1 clidx)
        let keyidx=idx_base+\$1
        if [ -f clientkeys-\$keyidx.tar.gz ]; then
            tar -C \$keydir -xf clientkeys-\$keyidx.tar.gz
            python3 \$updinitcfg -c \$keydir ./initcfg.json
            chmod 400 \$keydir/*.pem
        fi
        for i in clientkeys-*; do
            cat /dev/null > \$i
        done
    else
        idx_base=\$(head -n1 svridx)
        let keyidx=idx_base+\$1
        if [ -f serverkeys-\$keyidx.tar.gz ]; then
            tar -C \$keydir -xf serverkeys-\$keyidx.tar.gz
            python3 \$updinitcfg \$keydir ./initcfg.json
            chmod 400 \$keydir/*.pem
        fi
        for i in serverkeys-*; do
            cat /dev/null > \$i
        done
    fi
    rpcfgnui=/usr/bin/rpcf/rpcfgnui.py
    if [ ! -f \$rpcfgnui ]; then
        rpcfgnui=/usr/local/bin/rpcf/rpcfgnui.py
        if [ ! -f \$rpcfnui ];then
            exit 1
        fi
    fi
fi

initcfg=\$(pwd)/initcfg.json
if which sudo; then
    sudo python3 \$rpcfgnui ./initcfg.json
else
    su -c "python3 \$rpcfgnui \$initcfg"
fi
EOF

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
    mv clidx endidx
    tar rf instsvr.tar endidx
    mv endidx clidx
    tar rf instsvr.tar instcfg.sh
else
    rm -rf instsvr.tar
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
    mv rpcf_serial endidx
    tar rf instcli.tar endidx
    mv endidx rpcf_serial
    tar rf instcli.tar instcfg.sh
else
    rm -rf instcli.tar
fi 

else
    echo "GmSSL is not installed, and please install GmSSL first."
fi #which gmssl

popd
