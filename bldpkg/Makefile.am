EXTRA_DIST=control.in blddpkg.sh stripsym.sh

dpkg:
	if [ ! -e $(DESTDIR)/DEBIAN ]; then \
		mkdir -p $(DESTDIR)/DEBIAN;fi;\
	arch=$(host_cpu);\
	if [ "x$${arch}" == "xx86_64" ]; then arch="amd64";fi;\
	bash blddpkg.sh $(DESTDIR) $(PACKAGE_NAME)_$(VERSION)_$${arch}

install:
	if [ "x$(DESTDIR)" != "x" ]; then \
		if which dpkg-deb; then make dpkg; fi; \
	fi

