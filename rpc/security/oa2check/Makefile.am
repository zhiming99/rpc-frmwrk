lib_LIBRARIES=liboa2check.a

liboa2check_a_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/ipc -I../ \
-I$(top_srcdir)/combase -I$(top_srcdir)/rpc -I$(top_srcdir)/3rd_party

liboa2check_a_SOURCES = oa2check.cpp oa2check.h OA2proxycli.h OA2proxycli.cpp
liboa2check_adir=./

oa2check_confdir= $(sysconfdir)/rpcf
dist_oa2check_conf_DATA=oa2checkdesc.json
EXTRA_DIST=oa2check.ridl

liboa2check_a_CPPFLAGS += -fPIC

#libwsock_a_LDFLAGS=-version-info 0:0:0
liboa2check_a_LIBADD=
