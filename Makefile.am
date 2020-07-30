ACLOCAL_AMFLAGS = -I m4
bin_PROGRAMS = rpcfrmwrk
rpcfrmwrk_SOURCES = 

rpcfrmwrk:
	$(MAKE) -f Makefile.start debug

debug:
	$(MAKE) -f Makefile.start debug

release:
	$(MAKE) -f Makefile.start release

clean:
	$(MAKE) -f Makefile.start clean


release_clean:
	$(MAKE) -f Makefile.start release_clean

debug_clean:
	$(MAKE) -f Makefile.start debug_clean

.PHONY: debug release clean release_clean debug_clean rpcfrmwrk
