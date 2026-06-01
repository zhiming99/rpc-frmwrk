EXTRA_DIST=updconst.sh stscript.y stlexer.l stparser.y 


rpcfstc_PROGRAMS=rpcfstc

dep_files=lvalvar.h nonterm.h parsrctx.cpp parsrctx.h parsrdrv.cpp stast.h stbase.cpp stbase.h stclsids.h stmain.cpp 
rpcfstc_SOURCES= stparser.cpp stlexer.cpp stparser.h stlexer.h $(dep_files) 
rpcfstcdir=$(bindir)
rpcfstc_LDADD=../../combase/libcombase.la 

rpcfstc_CPPFLAGS= -I$(combase_dir) -I$(top_srcdir)/include -fPIC
combase_dir=$(top_srcdir)/combase

stlexer.%: stlexer.l stparser.h
	flex --header-file=stlexer.h -o stlexer.cpp stlexer.l

stparser.%: stparser.y
	bison --defines=stparser.h -Wcounterexamples --output=stparser.cpp -r all stparser.y
	bash updconst.sh

$(dep_files): stparser.h stlexer.h
