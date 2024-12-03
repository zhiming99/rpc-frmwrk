EXTRA_DIST=RegFsSvcLocalcli.py RegFsSvcLocalclibase.py oinitdesc.json RegistryFsstructs.py encdec.py oinit.py seribase.py weblogin.py __init__.py __main__.py setup.py.in \
debian/control debian/rules debian/copyright debian/postinst debian/preinst debian/prerm Makefile.deb README.md README_cn.md

rev=0
PACKAGE_NAME=python3-oinit

deb:
	echo buiding debian package...
	if [ ! -d debpkg/debian ]; then mkdir -p debpkg/debian;fi;
	echo "10" > debpkg/debian/compat
	echo 'usr/bin/rpcf/*.whl' > debpkg/debian/python3-oinit.install
	cp debian/{control,rules,copyright,preinst,postinst,prerm} debpkg/debian/
	if [ ! -d debpkg/oinit ]; then mkdir -p debpkg/oinit;fi; cp *.py debpkg/oinit;mv debpkg/oinit/setup.py debpkg/
	cp oinitdesc.json debpkg/oinit;cp Makefile.deb debpkg/Makefile;cp *.md debpkg/oinit/
	pushd debpkg;DEBEMAIL="zhiming <woodhead99@gmail.com>" dch --create -v $(VERSION)-$(rev) --package $(PACKAGE_NAME) "$(PACKAGE_NAME)-$(VERSION)-$(rev) released";\
 dpkg-buildpackage -us -uc;popd
	if [ ! -d output ]; then mkdir output; fi; mv python3-oinit* output/

.PHONY: deb

