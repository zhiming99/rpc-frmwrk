ridlc_PROGRAMS=ridlc
ridlc_SOURCES= gencpp.cpp astnode.cpp main.cpp astnode.h gencpp.h idlclsid.h genpy.cpp genpy.h example.ridl tinyif.ridl README.md odesctpl.json drvtpl.json mktpl.in synccfg.py.in pymktpl

EXTRA_DIST=ridlc.y ridlc.lex pytest/SimpFileSvcsvr.py pytest/SimpFileSvccli.py pytest/StreamSvcsvr.py pytest/StreamSvccli.py pytest/maincli.py pytest/mainsvr.py

nodist_ridlc_SOURCES = ridlc.cpp lexer.cpp ridlc.h lexer.h mktpl synccfg.py

BUILT_SOURCES=ridlc.cpp lexer.cpp mktpl synccfg.py 

CLEANFILES = ridlc.cpp lexer.cpp ridlc.h lexer.h mktpl

lexer.cpp lexer.h : ridlc.lex
	flex ridlc.lex

ridlc.cpp ridlc.h : ridlc.y
	bison --defines=ridlc.h --output=ridlc.cpp -r all ridlc.y

mktpl : mktpl.in
	sed -e 's,[@]PKG_CONFIG_PATH[@],$(pkgconfigdir),g;s,[@]DIST_LIB_DIR[@],$(libdir),g' mktpl.in > mktpl

synccfg.py : synccfg.py.in
	sed -e 's,[@]SRCCFG[@],$(ridlc_confdir)/echodesc.json,g' synccfg.py.in > synccfg.py

ridlc_LDADD=$(COMMON_LINK) -lipc -lcombase -lutils -lstdc++ -ljsoncpp -L../combase -L../ipc -L../3rd_party
ridlcdir=$(bindir)

dist_ridlc_conf_DATA=odesctpl.json drvtpl.json mktpl synccfg.py seribase.py pymktpl
ridlc_confdir=$(sysconfdir)/rpcf

include_ipcdir=$(top_srcdir)/ipc
include_ridlcdir=$(top_srcdir)/ridlc

link_dir=-L$(top_srcdir)/debug64/
ridlc_CPPFLAGS= -I$(include_ipcdir) -I$(top_srcdir)/include

