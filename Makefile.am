ACLOCAL_AMFLAGS = -I m4
bin_PROGRAMS = rpcfrmwrk
rpcfrmwrk_SOURCES = 

rpcfrmwrk:
	make -f Makefile.start debug

debug:
	make -f Makefile.start debug

release:
	make -f Makefile.start release

clean:
	make -f Makefile.start clean


release_clean:
	make -f Makefile.start release_clean

debug_clean:
	make -f Makefile.start debug_clean

.PHONY: debug release clean release_clean debug_clean rpcfrmwrk
