EXTRA_DIST=control.in blddpkg.sh rules bldrpm.sh rpcf.spec.in rpcf.install.in rpcf-dev.install.in rpcf-static.install.in README.md

rev=0
DISTDIR=$$(pwd)/$(PACKAGE_NAME)-$(VERSION)-$(rev)
if ENABLE_PYTHON
    ena_python="yes"
else
    ena_python="no"
endif

if ENABLE_JS
    ena_js="yes"
else
    ena_js="no"
endif

if ENABLE_JAVA
    ena_java="yes"
else
    ena_java="no"
endif

debdir:
	if [ ! -d $(DISTDIR)/debian/$(PACKAGE_NAME) ]; then \
		mkdir -p $(DISTDIR)/debian/$(PACKAGE_NAME);\
    fi;
	arch=$$(dpkg --print-architecture);\
	if ! bash blddpkg.sh $(DISTDIR) $(DESTDIR) $(PACKAGE_NAME)_$(VERSION)-$(rev)_$${arch} $(prefix) $(ena_python) $(ena_js) $(ena_java); then \
        echo error occurs && false; \
    else \
        cd $(DISTDIR);\
        DEBEMAIL="zhiming <woodhead99@gmail.com>" dch --create -v $(VERSION)-$(rev) --package $(PACKAGE_NAME) "$(PACKAGE_NAME)-$(VERSION)-$(rev) released";\
    fi

deb:
	if [ "x$(DESTDIR)" == "x" ]; then \
        echo 'please call this target from the master Makefile in the parent directory' \
        && false; \
	else \
        if which debuild; then \
            make debdir DESTDIR=$(DESTDIR);\
        fi;\
    fi
	for i in *.deb *.ddeb; do rm $$i; done || true
	cd $(DISTDIR);debuild -us -uc -d
	debian_ver=$$(cat /etc/os-release | grep VERSION_ID | awk -F '"' '{print $$2}');\
    debian_id=$$(cat /etc/os-release | grep '^ID=' | awk -F '=' '{print $$2}');\
    oid=$${debian_id:0:1};\
    echo debian OS version is $${debian_ver}; \
    debian_ver=$${oid}$${debian_ver%%.*};\
    if compgen -G "*.deb" > /dev/null; then \
        for i in *.deb; do \
            mv $$i $${i%%.deb}.$${debian_ver}.deb; \
        done;\
    fi;\
    if compgen -G "*.ddeb" > /dev/null; then \
        for i in *.ddeb; do \
            mv $$i $${i%%.ddeb}.$${debian_ver}.ddeb; \
        done;\
    fi;
if ENABLE_JS
	make -C ../monitor/oinit deb
	cp ../monitor/oinit/output/python3-oinit*.deb .
endif
	make -C ../monitor/client/python deb
	cp ../monitor/client/python/python3-appmoncli*.deb .

rpm:
	rm -rf $${HOME}/rpmbuild && rpmdev-setuptree
	make -C .. dist
	cp ../$(PACKAGE_NAME)-$(VERSION).tar.gz $${HOME}/rpmbuild/SOURCES
	bash ./bldrpm.sh $$(pwd)/../ $(PACKAGE_NAME) $(host_cpu)
if ENABLE_JS
	make -C ../monitor/oinit rpm
	cp ../monitor/oinit/output/python3-oinit*.rpm .
endif
	make -C ../monitor/client/python rpm
	cp ../monitor/client/python/python3-appmoncli*.rpm .
