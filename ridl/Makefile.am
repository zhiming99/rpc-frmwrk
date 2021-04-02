ridlc_PROGRAMS=ridlc
ridlc_SOURCES= gencpp.cpp astnode.cpp main.cpp astnode.h gencpp.h idlclsid.h example.ridl tinyif.ridl README.md odesctpl.json drvtpl.json mktpl

EXTRA_DIST=ridlc.y ridlc.lex

nodist_ridlc_SOURCES = ridlc.cpp lexer.cpp ridlc.h lexer.h
BUILT_SOURCES=ridlc.cpp lexer.cpp
CLEANFILES = ridlc.cpp lexer.cpp ridlc.h lexer.h

lexer.cpp lexer.h : ridlc.lex
	flex ridlc.lex

ridlc.cpp ridlc.h : ridlc.y
	bison --defines=ridlc.h --output=ridlc.cpp -r all ridlc.y

ridlc_LDADD=-lipc -lcombase -lutils -lstdc++ -ljsoncpp -L../combase -L../ipc -L../3rd_party
ridlcdir=$(bindir)

ridlc_conf_DATA=odesctpl.json drvtpl.json mktpl
ridlc_confdir=$(sysconfdir)/rpcf

include_ipcdir=$(top_srcdir)/ipc
include_ridlcdir=$(top_srcdir)/ridlc

link_dir=-L$(top_srcdir)/debug64/
ridlc_CPPFLAGS= -I$(include_ipcdir) -I$(top_srcdir)/include

