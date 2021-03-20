ridlc_PROGRAMS=ridlc
ridlc_SOURCES=ridlc.cpp lexer.cpp gencpp.cpp astnode.cpp main.cpp

lexer.cpp: ridlc.lex ridlc.h
	flex ridlc.lex

ridlc.cpp: ridlc.y
	bison --defines=ridlc.h --output=ridlc.cpp -r all ridlc.y

ridlc_LDADD=-lipc -lcombase -lutils -lstdc++ -L$(top_srcdir)/debug64
ridlcdir=./

dbus_INCFLAGS=`pkg-config --cflags dbus-1 jsoncpp`
ridlc_CXXFLAGS=$(dbus_INCFLAGS) -D_USE_LIBEV=1 -O0 -g -ggdb
ridlc_HEADERS=gencpp.h astnode.h idlclsid.h

include_rpcdir=$(top_srcdir)/include
include_comdir=$(top_srcdir)/combase
include_ipcdir=$(top_srcdir)/ipc
include_ridlcdir=$(top_srcdir)/ridlc

link_dir=-L$(top_srcdir)/debug64/
ridlc_CPPFLAGS= -I$(include_rpcdir) -I$(include_ipcdir) -I$(top_srcdir)/3rd_party/

