#!/bin/bash
DISTDIR=$1
DEBDIR=${DISTDIR}/debian
DESTDIR=$2
pkgname=$3
prefix=$4

#the script can run only on debian family of system
os_name=`cat /etc/os-release | grep '^ID_LIKE' | awk -F '=' '{print $2}'`
if [ "${os_name}" != "debian" ]; then
    exit 1
fi

sed -i 's/x86_64/amd64/g' ./control;

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


echo '13' > ${DEBDIR}/compat
echo '3.0 (quilt)' > ${DEBDIR}/source/format

cat << EOF >> ${DEBDIR}/postinst
#!/bin/bash
for i in *.whl; do
pip3 install XXXX/${PACKAGE_NAME}/\${i};
done
#DEBHELPER#
EOF

cat << EOF >> ${DEBDIR}/postrm
#!/bin/bash
pip3 uninstall -y ${PACKAGE_NAME}
#DEBHELPER#
EOF

chmod 0755 ${DEBDIR}/postinst
chmod 0755 ${DEBDIR}/postrm