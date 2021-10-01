EXTRA_DIST=control.in blddpkg.sh rules bldrpm.sh rpcf.spec.in
rev=0
DISTDIR=$$(pwd)/$(PACKAGE_NAME)-$(VERSION)-$(rev)

debdir:
	if [ ! -d $(DISTDIR)/debian/$(PACKAGE_NAME) ]; then \
		mkdir -p $(DISTDIR)/debian/$(PACKAGE_NAME);\
    fi;
	arch=$(host_cpu);\
	if [ "x$${arch}" == "xx86_64" ]; then arch="amd64";fi;\
	if ! bash blddpkg.sh $(DISTDIR) $(DESTDIR) $(PACKAGE_NAME)_$(VERSION)-$(rev)_$${arch} $(prefix); then \
        echo error occurs && false; \
    else \
        cd $(DISTDIR);\
        dch --create -v $(VERSION)-$(rev) --package $(PACKAGE_NAME);\
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
	ubuntu_ver=$$(cat /etc/os-release | grep VERSION_ID | awk -F '"' '{print $$2}');\
    echo ubuntu version is $${ubuntu_ver}; \
    if [[ "$${ubuntu_ver}" > "21.00" ]]; then \
        ubuntu_ver="u"$${ubuntu_ver%%.*}; \
    else \
        ubuntu_ver="u20";\
    fi;\
    for i in *.deb; do \
        mv $$i $${i%%.deb}.$${ubuntu_ver}.deb; \
    done;\
    for i in *.ddeb; do \
        mv $$i $${i%%.ddeb}.$${ubuntu_ver}.ddeb; \
    done

rpm:
	rm -rf $${HOME}/rpmbuild && rpmdev-setuptree
	make -C .. dist
	cp ../$(PACKAGE_NAME)-$(VERSION).tar.gz $${HOME}/rpmbuild/SOURCES
	bash ./bldrpm.sh $$(pwd)/../ $(PACKAGE_NAME) $(host_cpu)
