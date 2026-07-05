#!/bin/bash
if [ ! -z "$1" ];then
    destfile=$1
else
    destfile=./rpcf-source.tar.gz
fi

if [ ! -z "$2" ];then
    prefix=$2
else
    prefix=
fi
if ! git archive --format=tar.gz --prefix=$prefix/ HEAD -o $destfile; then
    exit $?
fi

