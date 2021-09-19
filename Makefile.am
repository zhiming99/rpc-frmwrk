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

EXTRA_DIST= README.md cfgsel

dist_pkgconfig_DATA=rpcf.pc
