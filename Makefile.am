ACLOCAL_AMFLAGS= -I m4
SUBDIRS=3rd_party combase ipc rpc ridl tools pics

if ENABLE_TESTCASES
SUBDIRS += test
endif

if SIPBUILD
SUBDIRS += python
endif

if SIP4                                                                 
SUBDIRS += python                                                       
endif                          

SUBDIRS += bldpkg

EXTRA_DIST= README.md cfgsel Concept.md LICENSE

dist_pkgconfig_DATA=rpcf.pc

DISTDIR=$(PACKAGE_NAME)-$(VERSION)
pkgname=$(DISTDIR).tar.gz
debtar=$(subst -,_,$(DISTDIR)).orig.tar.gz
deb:
	make dist;echo dist done ; mv $(pkgname) ./bldpkg/${debtar};cd bldpkg;tar zxf $(debtar)
	make -C ./bldpkg deb DESTDIR=`pwd`/bldpkg/$(DISTDIR)/debian/$(PACKAGE_NAME)
	@rm -rf ./bldpkg/$(DISTDIR)
