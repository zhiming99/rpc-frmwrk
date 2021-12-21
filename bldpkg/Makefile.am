EXTRA_DIST=control.in blddpkg.sh rules bldrpm.sh rpcf.spec.in rpcf.install.in rpcf-dev.install.in rpcf-static.install.in README.md

rev=0
DISTDIR=$$(pwd)/$(PACKAGE_NAME)-$(VERSION)-$(rev)

debdir:
	if [ ! -d $(DISTDIR)/debian/$(PACKAGE_NAME) ]; then \
		mkdir -p $(DISTDIR)/debian/$(PACKAGE_NAME);\
    fi;
	arch=$$(dpkg --print-architecture);\
	if ! bash blddpkg.sh $(DISTDIR) $(DESTDIR) $(PACKAGE_NAME)_$(VERSION)-$(rev)_$${arch} $(prefix); then \
        echo error occurs && false; \
    else \
        cd $(DISTDIR);\
        DEBEMAIL="zhiming <woodhead99@gmail.com>" dch --create -v $(VERSION)-$(rev) --package $(PACKAGE_NAME) "$(PACKAGE_NAME)-$(VERSION)-$(rev) released";\
    fi

deb:
	if [ "x$(DESTDIR)" == "x" ]; \
        then echo done && false; \
	else \
        if which debuild; then \
            make debdir DESTDIR=$(DESTDIR);\
        fi;\
    fi
	for i in *.deb *.ddeb; do rm $$i; done || true
	cd $(DISTDIR);debuild -us -uc
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

rpm:
	rm -rf $${HOME}/rpmbuild && rpmdev-setuptree
	make -C .. dist
	cp ../$(PACKAGE_NAME)-$(VERSION).tar.gz $${HOME}/rpmbuild/SOURCES
	bash ./bldrpm.sh $$(pwd)/../ $(PACKAGE_NAME) $(host_cpu)
