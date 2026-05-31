EXTRA_DIST= stparser.y stlexer.l stbase.cpp stscript.y parsrctx.cpp parsrctx.h stast.h nonterm.h stmain.cpp stclsids.h lvalvar.h stbase.h updconst.sh parsrdrv.cpp

stmain.cpp:stparser.cpp

stlexer.cpp stlexer.h: stlexer.l
	flex --header-file=stlexer.h -o stlexer.cpp stlexer.l

stparser.cpp stparser.h: stparser.y
	bison --defines=stparser.h -Wcounterexamples --output=stparser.cpp -r all stparser.y
	bash updconst.sh

rpcfstc_PROGRAMS=rpcfstc
rpcfstc_SOURCES=stlexer.cpp stlexer.h stparser.cpp stparser.h stbase.cpp stmain.cpp stast.h nonterm.h stbase.h parsrctx.cpp parsrdrv.cpp
rpcfstcdir=$(bindir)
rpcfstc_LDADD=../../combase/libcombase.la 

rpcfstc_CPPFLAGS= -I$(combase_dir) -I$(top_srcdir)/include -fPIC
combase_dir=$(top_srcdir)/combase
