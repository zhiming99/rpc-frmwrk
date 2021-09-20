EXTRA_DIST=control.in

dpkg:
	if [ ! -e $(DESTDIR)/DEBIAN ]; then \
		mkdir -p $(DESTDIR)/DEBIAN;fi;\
	bash blddpkg.sh $(DESTDIR)

install:
	if [ ! "x$(DESTDIR)" == "x" ]; then \
		if which dpkg-deb; then make dpkg; fi; \
	fi

