EXTRA_DIST=RegFsSvcLocalcli.py RegFsSvcLocalclibase.py oinitdesc.json \
RegistryFsstructs.py encdec.py oinit.py seribase.py weblogin.py __init__.py __main__.py setup.py.in \
debian/control debian/rules debian/copyright debian/postinst debian/preinst \
debian/prerm Makefile.deb README.md README_cn.md oinit.spec.in

rev=0
PACKAGE_NAME=python3-oinit

deb:
	echo buiding debian package...
	if [ -d debpkg ]; then rm -rf debpkg; fi
	mkdir -p debpkg/debian
	echo "10" > debpkg/debian/compat
	echo 'usr/lib/rpcf/*.whl' > debpkg/debian/python3-oinit.install
	cp debian/{control,rules,copyright,preinst,postinst,prerm} debpkg/debian/
	if [ ! -d debpkg/oinit ]; then mkdir -p debpkg/oinit;fi; cp *.py debpkg/oinit;mv debpkg/oinit/setup.py debpkg/
	cp oinitdesc.json debpkg/oinit;cp Makefile.deb debpkg/Makefile;cp *.md debpkg/oinit/
	pushd debpkg;DEBEMAIL="zhiming $(PACKAGE_BUGREPORT)" dch --create -v $(VERSION)-$(rev) --package $(PACKAGE_NAME) "$(PACKAGE_NAME)-$(VERSION)-$(rev) released";\
 dpkg-buildpackage -us -uc;popd
	if [ -d output ]; then rm output/python3-oinit*deb > /dev/null || true;fi
	if [ ! -d output ]; then mkdir output; fi; mv python3-oinit* output/
	echo deb package is generated in 'output' directory.

rpm:
	echo building rpm package...
	if [ -d $${HOME}/oinitrpm ];then rm -rf $${HOME}/oinitrpm;fi
	if [ ! -d $${HOME}/oinitrpm ]; then mkdir -p $${HOME}/oinitrpm/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS};fi
	if [ -d rpmpkg ]; then rm -rf rpmpkg; fi
	if [ ! -d rpmpkg/oinit ]; then mkdir -p rpmpkg/oinit;fi;
	cp *.py rpmpkg/oinit;mv rpmpkg/oinit/setup.py rpmpkg/; cp README* rpmpkg/oinit; cp oinitdesc.json rpmpkg/oinit
	pushd rpmpkg; python3 setup.py bdist_wheel; cp dist/*.whl $${HOME}/oinitrpm/SOURCES/;popd
	cp oinit.spec $${HOME}/oinitrpm/SPECS
	pushd $${HOME}/oinitrpm/;if rpmbuild -bb --define "_topdir `pwd`" SPECS/oinit.spec; then cp ./RPMS/noarch/*.rpm $(abs_builddir);fi;popd
	rm -rf $${HOME}/oinitrpm; rm -rf rpmpkg
	if [ -d output ]; then rm output/python3-oinit*rpm;fi
	if [ ! -d output ]; then mkdir output; fi; mv python3-oinit*.rpm output/
	echo rpm package is generated in 'output' directory.

wheel:
	if [ ! -d output ]; then mkdir output; fi
	if [ -d rpmpkg ]; then rm -rf rpmpkg; fi
	if [ ! -d rpmpkg/oinit ]; then mkdir -p rpmpkg/oinit;fi;
	cp *.py rpmpkg/oinit;mv rpmpkg/oinit/setup.py rpmpkg/; cp README* rpmpkg/oinit; cp oinitdesc.json rpmpkg/oinit
	pushd rpmpkg; python3 setup.py bdist_wheel; cp dist/*.whl ../output;popd
	rm -rf rpmpkg
	echo wheel package is generated in 'output' directory.

localinst:
	make wheel
	pip3 install output/*.whl

.PHONY: deb rpm wheel localinst

