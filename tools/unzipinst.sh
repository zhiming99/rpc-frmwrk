#!/bin/bash
if [[ -z "$1" ]] || [[ -z "$2" ]]; then
    echo "Usage: unzipinst <installer name> <path>"
    exit 1
else
    unzipdir=$2
fi
GZFILE=`awk '/^__GZFILE__/ {print NR + 1; exit 0; }' $1`
if [[ -z "$GZFILE" ]]; then
    echo Invalid installer format
    exit 1
fi
tail -n+$GZFILE $1 | tar -zxv -C $unzipdir > /dev/null 2>&1
if (( $? == 0 )); then
    echo unzip success;
else
    echo unzip failed;
    exit 1;
fi
pushd $unzipdir > /dev/null; ls -l; popd > /dev/null
exit 0


