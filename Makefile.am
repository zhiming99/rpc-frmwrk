ACLOCAL_AMFLAGS= -I m4
SUBDIRS=3rd_party combase ipc rpc ridl tools pics

if ENABLE_TESTCASES
SUBDIRS += test
endif

SUBDIRS += python

EXTRA_DIST= README.md cfgdbg

dist_pkgconfig_DATA=rpcf.pc
