EXTRA_DIST= stparser.y stlexer.l stbase.cpp stscript.y parsrctx.cpp parsrctx.h stast.h nonterm.h stmain.cpp stclsids.h lvalvar.h

stmain.cpp:stparser.cpp

stlexer.cpp stlexer.h: stlexer.l
	flex --header-file=stlexer.h -o stlexer.cpp stlexer.l

stparser.cpp stparser.h: stparser.y
	bison --defines=stparser.h -Wcounterexamples --output=stparser.cpp -r all stparser.y
	@stateNo=`awk '/State ([0-9]{3}|[0-9]{2}|[0-9])/ { last_match = $$2; } /case_list_selector cinner_statements \xE2\x80\xA2 TOK_VCASE_SEP/ { print last_match; exit }' stparser.output`;\
    sed -i "s/define CONFLICT_STATE .*/define CONFLICT_STATE     $$stateNo/" parsrctx.h;echo CONFLICT_STATE=$$stateNo;\
    stateNo=`awk '/State ([0-9]{3}|[0-9]{2}|[0-9])/ { last_match = $$2; } /arithmetic_expr: arithmetic_expr \xE2\x80\xA2 TOK_ADD/ { print last_match; exit }' stparser.output`;\
    sed -i "s/define SUBEXPR_STATE .*/define SUBEXPR_STATE     $$stateNo/" parsrctx.h; echo SUBEXPR_STATE=$$stateNo;\
    stateNo=`awk '/State ([0-9]{3}|[0-9]{2}|[0-9])/ { last_match = $$2; } /l_value \xE2\x80\xA2 TOK_VPUNC TOK_NUMBER/ { printf( "%d, ",last_match ); }' stparser.output`;\
    stateNo="$${stateNo%, }";sed -i "s/define LVALUE_BIT_STATES .*/define LVALUE_BIT_STATES     std::vector({ $${stateNo} })/" parsrctx.h; echo LVALUE_BIT_STATES=$$stateNo;\
    stateNo=`awk '/State ([0-9]{3}|[0-9]{2}|[0-9])/ { last_match = $$2; } /TOK_METHOD .*instance_path \xE2\x80\xA2 TOK_VSEMICOLON/ { printf( "%d, ",last_match ); }' stparser.output`;\
    stateNo="$${stateNo%, }";sed -i "s/define METHOD_RET_STATE.*/define METHOD_RET_STATE     $$stateNo/" parsrctx.h;echo METHOD_RET_STATE=$$stateNo;
 
#rpcfstc_PROGRAMS=rpcfstc
rpcfstc_SOURCES=stlexer.cpp stlexer.h stparser.cpp stparser.h stbase.cpp stmain.cpp stast.h nonterm.h 
rpcfstcdir=$(bindir)

rpcfstc_CPPFLAGS= -I$(combase_dir) -I$(top_srcdir)/include -fPIC
combase_dir=$(top_srcdir)/combase
