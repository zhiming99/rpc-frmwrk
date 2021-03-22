ridlc_PROGRAMS=ridlc
ridlc_SOURCES=ridlc.cpp lexer.cpp gencpp.cpp astnode.cpp main.cpp

lexer.cpp: ridlc.lex ridlc.h
	flex ridlc.lex

ridlc.cpp: ridlc.y
	bison --defines=ridlc.h --output=ridlc.cpp -r all ridlc.y

ridlc_LDADD=-lipc -lcombase -lutils -lstdc++ -L../combase -L../ipc -L../3rd_party
ridlcdir=$(bindir)

#ridlc_HEADERS=gencpp.h astnode.h idlclsid.h

include_ipcdir=$(top_srcdir)/ipc
include_ridlcdir=$(top_srcdir)/ridlc

link_dir=-L$(top_srcdir)/debug64/
ridlc_CPPFLAGS= -I$(include_ipcdir)

