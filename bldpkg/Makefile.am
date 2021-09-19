EXTRA_DIST=control.in

dpkg:
	if [ ! -e $(DESTDIR)/DEBIAN ]; then \
		mkdir -p $(DESTDIR)/DEBIAN;fi;\
	cp ./control $(DESTDIR)/DEBIAN/;\
	dpkg-deb $(DESTDIR) .

install:
	if [ ! "x$(DESTDIR)" == "x" ]; then \
		if which dpkg-deb; then make dpkg; fi; \
	fi

