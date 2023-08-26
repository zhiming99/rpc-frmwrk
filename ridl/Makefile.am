SUBDIRS= rpcfs .

ridlc_PROGRAMS=ridlc
ridlc_SOURCES= gencpp.cpp astnode.cpp main.cpp astnode.h gencpp.h idlclsid.h \
genpy.cpp genpy.h fulltest.ridl tinyif.ridl README.md odesctpl.json \
odesctpl2.json drvtpl.json mktpl.in synccfg.py.in pymktpl genjava.cpp genjava.h \
genfuse.h genfuse.cpp gencpp2.h gencpp2.cpp genfuse2.h genfuse2.cpp genrpcfs.cpp

EXTRA_DIST=ridlc.y ridlc.lex pytest/SimpFileSvcsvr.py pytest/SimpFileSvccli.py pytest/StreamSvcsvr.py pytest/StreamSvccli.py pytest/maincli.py pytest/mainsvr.py ridlc.1 ridl.7 seribase.py JavaSerialBase.java JavaSerialImpl.java

nodist_ridlc_SOURCES = ridlc.cpp lexer.cpp ridlc.h lexer.h mktpl synccfg.py

BUILT_SOURCES=ridlc.cpp lexer.cpp mktpl synccfg.py 

man_MANS=ridlc.1 ridl.7

CLEANFILES = ridlc.cpp lexer.cpp ridlc.h lexer.h mktpl

lexer.cpp lexer.h : ridlc.lex
	flex ridlc.lex

ridlc.cpp ridlc.h : ridlc.y
	bison --defines=ridlc.h --output=ridlc.cpp -r all ridlc.y

mktpl : mktpl.in
	sed -e 's,[@]PKG_CONFIG_PATH[@],$(pkgconfigdir),g;s,[@]DIST_LIB_DIR[@],$(libdir),g;s,[@]BITWIDTH[@],$(BITWIDTH),g;' mktpl.in > mktpl

synccfg.py : synccfg.py.in
	sed -e 's,[@]SRCCFG[@],$(ridlc_confdir)/echodesc.json,g' synccfg.py.in > synccfg.py
	sed -i -e 's,[@]BINPATH[@],$(bindir)/rpcf,g' synccfg.py

ridlc_LDADD=$(COMMON_LINK) ../combase/libcombase.la ./rpcfs/libgenrpcfs.a -lutils -lstdc++ -ljsoncpp -L../combase -L../ipc -L../3rd_party
ridlcdir=$(bindir)

ridlc_conf_DATA=odesctpl.json odesctpl2.json drvtpl.json mktpl seribase.py pymktpl synccfg.py JavaSerialBase.java JavaSerialImpl.java
ridlc_confdir=$(sysconfdir)/rpcf

include_ipcdir=$(top_srcdir)/ipc
include_ridlcdir=$(top_srcdir)/ridlc

link_dir=-L$(top_srcdir)/debug64/
ridlc_CPPFLAGS= -I$(include_ipcdir) -I$(top_srcdir)/include -fPIC
if ENABLE_FUSE3
ridlc_CPPFLAGS+= -DFUSE3
endif

if ENABLE_JAVA
ridlc_CPPFLAGS+= -DJAVA
endif

if ENABLE_PYTHON
ridlc_CPPFLAGS+= -DPYTHON
endif


if ENABLE_KRB5
ridlc_CPPFLAGS+= -DKRB5 
endif

if ENABLE_AUTH
ridlc_CPPFLAGS+= -DAUTH
endif

if ENABLE_OPENSSL
ridlc_CPPFLAGS+= -DOPENSSL
endif

if ENABLE_GMSSL
ridlc_CPPFLAGS+= -DGMSSL
endif

