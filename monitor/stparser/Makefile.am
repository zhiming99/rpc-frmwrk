EXTRA_DIST=updconst.sh stscript.y stlexer.l stparser.y CMakeLists.txt \
test_ast.cpp

BUILT_SOURCES = stparser.h stlexer.h
CLEANFILES = stparser.cpp stparser.h stlexer.cpp stlexer.h

check_PROGRAMS=rpcfstc
dep_files=lvalvar.h nonterm.h parsrctx.cpp parsrctx.h parsrdrv.cpp stast.h \
stbase.cpp stbase.h stclsids.h stmain.cpp astnodes.cpp astnodes.h astfactory.h \
astbuilder.h

rpcfstc_SOURCES= stparser.cpp stlexer.cpp stparser.h stlexer.h $(dep_files) \
stgenbase.cpp stgenbase.h

rpcfstcdir=$(bindir)
rpcfstc_LDADD=../../combase/libcombase.la 

rpcfstc_CPPFLAGS= -I$(combase_dir) -I$(top_srcdir)/include -fPIC
combase_dir=$(top_srcdir)/combase

stlexer.cpp: stlexer.l stparser.h
	flex --header-file=stlexer.h -o stlexer.cpp stlexer.l

stlexer.h: stlexer.cpp

stparser.cpp: stparser.y
	LANG=C bison --defines=stparser.h -Wcounterexamples --output=stparser.cpp -r all stparser.y
	bash updconst.sh

stparser.h: stparser.cpp

$(dep_files): stparser.h stlexer.h
