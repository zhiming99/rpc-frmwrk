EXTRA_DIST=RegFsSvcLocalcli.py RegFsSvcLocalclibase.py oinitdesc.json RegistryFsstructs.py encdec.py oinit.py seribase.py weblogin.py __init__.py __main__.py setup.py.in \
debian/python3-oinit.install debian/control debian/compat debian/rules debian/copyright debian/postinst debian/preinst debian/prerm debian/changelog Makefile.deb README.md README_cn.md

deb:
	echo buiding debian package...
	if [ ! -d debpkg/debian ]; then mkdir -p debpkg/debian;fi;
	cp debian/{control,rules,changelog,compat,copyright,preinst,postinst,python3-oinit.install,prerm} debpkg/debian/
	if [ ! -d debpkg/oinit ]; then mkdir -p debpkg/oinit;fi; cp *.py debpkg/oinit;mv debpkg/oinit/setup.py debpkg/
	cp oinitdesc.json debpkg/oinit;cp Makefile.deb debpkg/Makefile;cp *.md debpkg/oinit/
	pushd debpkg;dpkg-buildpackage -us -uc;popd
	if [ ! -d output ]; then mkdir output; fi; mv python3-oinit* output/

.PHONY: deb

