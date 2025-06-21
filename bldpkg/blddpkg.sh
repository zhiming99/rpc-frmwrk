#!/bin/bash
DISTDIR=$1
DEBDIR=${DISTDIR}/debian
DESTDIR=$2
pkgname=$3
prefix=$4
BUILD_PYTHON=$5
BUILD_JS=$6
BUILD_JAVA=$7

#the script can run only on debian family of system
os_name=`cat /etc/os-release | grep '^ID_LIKE' | awk -F '=' '{print $2}'`
os_name2=`cat /etc/os-release | grep '^ID=' | awk -F '=' '{print $2}'`
if [[ "${os_name}" != "debian" && "${os_name2}" != "debian" ]]; then 
    echo not debian system 
    exit 1 
fi

if [ ! -d ${DEBDIR}/source ]; then mkdir -p ${DEBDIR}/source;fi
cp ./control ${DEBDIR}
cp ./rules ${DEBDIR}

PACKAGE_NAME=${pkgname%%_*}

cat << EOF > ${DEBDIR}/copyright
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: ${PACKAGE_NAME}
Upstream-Contact: zhiming <woodhead99@gmail.com>
Source:https://github.com/zhiming99/rpc-frmwrk

Files: *
Copyright: 2021 Zhi Ming <woodhead99@gmail.com>
License: GPL-3+
Comment: GPL-3 is the default license for all the files except those in 3rd_party directory which have their own license disclaimer. There could also have some exceptions in other directories. Please refer to the file banner for detail information.

License: GPL-3+
 This library is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 .
 This package is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 .
 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>
 .
 On Debian systems, the complete text of the GNU General
 Public License version 3 can be found in "/usr/share/common-licenses/GPL-3".

EOF


DEB_HOST_MULTIARCH=$(dpkg-architecture -qDEB_HOST_MULTIARCH)

echo '13' > ${DEBDIR}/compat
echo '3.0 (quilt)' > ${DEBDIR}/source/format

force_inst=$(os_name=$(cat /etc/issue | awk '{print $1}');\
if [ "x${os_name}" == "xDebian" ]; then \
    os_version=$(cat /etc/issue | awk '{print $3}');\
    echo $(if ((${os_version} > 11 ));then echo '--break-system-packages';else echo ''; fi); \
elif [ "x${os_name}" == "xUbuntu" ]; then \
    os_version=$(cat /etc/issue | awk '{print $2}' | awk -F'.' '{print $1}');\
    echo $(if ((${os_version} > 22 ));then echo '--break-system-packages';else echo ''; fi); \
fi)

pushd ${DEBDIR}
if [ "x$BUILD_PYTHON" == "xyes" ]; then
cat << EOF >> ./postinst1
cd XXXX/${PACKAGE_NAME}
pypkg=\$(compgen -G "rpcf*whl")
pip3 install \$pypkg ${force_inst}
EOF
cat << EOF >> ./postrm1
pip3 uninstall -y ${PACKAGE_NAME} ${force_inst}
EOF
fi

if [ "x$BUILD_JS" == "xyes" ]; then
cat << EOF >> ./postinst2
echo "\e[33mnpm -g install assert browserify buffer exports long lz4 process put safe-buffer stream xxhashjs xxhash webpack minify webpack-cli vm events crypto-browserify stream-browserify\e[0m"
EOF
cat << EOF >> ./postrm2
echo  "\e[33mnpm -g remove assert browserify buffer exports long lz4 process put safe-buffer stream xxhashjs xxhash webpack minify webpack-cli vm events crypto-browserify stream-browserify\e[0m"
EOF
fi

if [ "x$BUILD_JAVA" == "xyes" ]; then
cat << 'EOF' >> ./postinst3
echo INSTALL_DIR=$(dpkg-query -L $DPKG_MAINTSCRIPT_PACKAGE | grep 'rpcbase.*jar' | head -n 1 | xargs dirname)
INSTALL_DIR=$(dpkg-query -L $DPKG_MAINTSCRIPT_PACKAGE | grep 'rpcbase.*jar' | head -n 1 | xargs dirname)
pushd $INSTALL_DIR
rpcbasejar=$(compgen -G "rpcbase*jar")
rm -f rpcbase.jar
ln -s $rpcbasejar rpcbase.jar
appmanclijar=$(compgen -G "appmancli*jar")
rm -f appmancli.jar
ln -s $appmanclijar appmancli.jar
popd
EOF

cat << 'EOF' >> ./prerm
#!/bin/bash
echo INSTALL_DIR=$(dpkg-query -L $DPKG_MAINTSCRIPT_PACKAGE | grep 'rpcbase.*jar' | head -n 1 | xargs dirname)
INSTALL_DIR=$(dpkg-query -L $DPKG_MAINTSCRIPT_PACKAGE | grep 'rpcbase.*jar' | head -n 1 | xargs dirname)
pushd $INSTALL_DIR
rm -f rpcbase.jar
rm -f appmancli.jar
popd
EOF
fi

if [ -f ./postinst1 -o -f ./postinst2 -o -f ./postinst3 ];then
    echo '#!/bin/bash' >> ./postinst
    echo '#!/bin/bash' >> ./postrm
    if [ -f ./postinst1 ]; then
        cat ./postinst1 >> ./postinst
        cat ./postrm1 >> ./postrm
        rm ./postinst1 ./postrm1 
    fi
    if [ -f ./postinst2 ]; then
        echo >> ./postinst
        echo >> ./postrm
        cat ./postinst2 >> ./postinst
        cat ./postrm2 >> ./postrm
        rm ./postinst2 ./postrm2 
    fi
    if [ -f ./postinst3 ];then
        echo >> ./postinst
        cat ./postinst3 >> ./postinst
        rm ./postinst3
    fi
    echo '#DEBHELPER#' >> ./postinst
    echo '#DEBHELPER#' >> ./postrm
    if [ -f ./prerm ]; then
        echo '#DEBHELPER#' >> ./prerm
    fi
else
    touch ./postrm
    touch ./postinst
fi
popd

sed -i "s:ZZZZZ:\n:g" rpcf.install
sed -i "s:ZZZZZ:\n:g" rpcf-dev.install
sed "s/[@]DEB_HOST_MULTIARCH[@]/${DEB_HOST_MULTIARCH}/g" rpcf.install > ${DEBDIR}/rpcf.install
sed "s/[@]DEB_HOST_MULTIARCH[@]/${DEB_HOST_MULTIARCH}/g" rpcf-dev.install > ${DEBDIR}/rpcf-dev.install
sed "s/[@]DEB_HOST_MULTIARCH[@]/${DEB_HOST_MULTIARCH}/g" rpcf-static.install.in > ${DEBDIR}/rpcf-static.install

chmod 0755 ${DEBDIR}/postinst
chmod 0755 ${DEBDIR}/postrm
