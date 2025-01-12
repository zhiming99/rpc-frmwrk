ACLOCAL_AMFLAGS= -I m4
SUBDIRS=3rd_party combase ipc logger docs

if ENABLE_FUSE3
SUBDIRS += fuse
endif

SUBDIRS += ridl monitor rpc tools pics examples 

SUBDIRS += test

if SWIG
if ENABLE_JAVA
SUBDIRS += java
endif
endif

if SIPBUILD
if ENABLE_PYTHON
SUBDIRS += python
endif
endif

if SIP4                                                                 
if ENABLE_PYTHON
SUBDIRS += python                                                       
endif
endif                          

if ENABLE_JS
SUBDIRS += js
endif

SUBDIRS += bldpkg

EXTRA_DIST= README.md cfgsel Concept.md logs.md LICENSE README_cn.md Concept_cn.md

dist_pkgconfig_DATA=rpcf.pc

rev=0
DISTDIR=$(PACKAGE_NAME)-$(VERSION)
pkgname=$(DISTDIR).tar.gz
debtar=$(subst -,_,$(DISTDIR)).orig.tar.gz
deb:
	make dist;echo dist done ; mv $(pkgname) ./bldpkg/${debtar};cd bldpkg;tar zxf $(debtar);\
	if [ -d $(DISTDIR)-$(rev) ]; then rm -rf $(DISTDIR)-$(rev); fi;mv $(DISTDIR) $(DISTDIR)-$(rev)
	make -C ./bldpkg deb DESTDIR=`pwd`/bldpkg/$(DISTDIR)-$(rev)/debian/$(PACKAGE_NAME)
	@rm -rf ./bldpkg/$(DISTDIR)-$(rev)

rpm:
	make -C ./bldpkg rpm
