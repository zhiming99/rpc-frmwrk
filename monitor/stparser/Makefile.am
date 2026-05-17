EXTRA_DIST= stparser.y stlexer.l stbase.cpp stscript.y parsrctx.cpp parsrctx.h stast.h nonterm.h stmain.cpp stclsids.h

stlexer.cpp stlexer.h: stlexer.l
	flex --header-file=stlexer.h -o stlexer.cpp stlexer.l

stparser.cpp stparser.h: stparser.y
	bison --defines=stparser.h -Wcounterexamples --output=stparser.cpp -r all stparser.y
	@lines=`grep 'State [[:digit:]]\+ conflicts' stparser.output | wc -l`;\
	if(( lines != 1 )); then \
        echo "one conflict and only one conflict expected, but there are $$lines conflicts in the rules";\
        exit 1;\
    fi; \
    stateNo=`grep 'State [[:digit:]]\+ conflicts' stparser.output|awk '{print $$2}'`;\
    if (( stateNo > 0 )); then \
        sed -i "s/define CONFLICT_STATE .*/define CONFLICT_STATE $$stateNo/" parsrctx.h; \
    fi; echo conflict state is $$stateNo, an expected conflict.
