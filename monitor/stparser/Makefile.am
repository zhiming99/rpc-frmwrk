EXTRA_DIST= stparser.y stlexer.l stbase.cpp stscript.y parsrctx.cpp parsrctx.h stast.h nonterm.h stmain.cpp stclsids.h

stlexer.cpp stlexer.h: stlexer.l
	flex --header-file=stlexer.h -o stlexer.cpp stlexer.l

stparser.cpp stparser.h: stparser.y
	bison --defines=stparser.h -Wcounterexamples --output=stparser.cpp -r all stparser.y
	@stateNo=`awk '/State ([0-9]{3}|[0-9]{2}|[0-9])/ { last_match = $$2; } /cinner_statements: case_body \xE2\x80\xA2  .TOK_SEMICOLON/ { print last_match; exit }' stparser.output`;\
    sed -i "s/define CONFLICT_STATE .*/define CONFLICT_STATE     $$stateNo/" parsrctx.h;\
    stateNo=`awk '/State ([0-9]{3}|[0-9]{2}|[0-9])/ { last_match = $$2; } /arithmetic_expr: arithmetic_expr \xE2\x80\xA2 TOK_ADD/ { print last_match; exit }' stparser.output`;\
    sed -i "s/define SUBEXPR_STATE .*/define SUBEXPR_STATE     $$stateNo/" parsrctx.h; \
    stateNo=`awk '/State ([0-9]{3}|[0-9]{2}|[0-9])/ { last_match = $$2; } /l_value \xE2\x80\xA2 TOK_VPUNC TOK_NUMBER/ { printf( "%d, ",last_match ); }' stparser.output`;\
    stateNo="$${stateNo%, }";sed -i "s/define LVALUE_BIT_STATES .*/define LVALUE_BIT_STATES     std::set({ $${stateNo} })/" parsrctx.h; \
    stateNo=`awk '/State ([0-9]{3}|[0-9]{2}|[0-9])/ { last_match = $$2; } /derived_type \xE2\x80\xA2 TOK_SEMICOLON var_declarations/ { printf( "%d, ",last_match ); }' stparser.output`;\
    stateNo="$${stateNo%, }";sed -i "s/define METHOD_RET_STATE .*/define METHOD_RET_STATE     std::set({ $${stateNo} })/" parsrctx.h;
 
