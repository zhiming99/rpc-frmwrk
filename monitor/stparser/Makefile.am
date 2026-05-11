EXTRA_DIST= stparser.y stlexer.l stbase.cpp stscript.y parsrctx.cpp parsrctx.h stast.h nonterm.h

stlexer.cpp stlexer.h: stlexer.l
	flex --header-file=stlexer.h -o stlexer.cpp stlexer.l

stscript.cpp stscript.h: stscript.y
	bison --defines=stscript.h -Wcounterexamples --output=stscript.cpp -r all stscript.y
	lines=`grep 'State [[:digit:]]\+ conflicts' stscript.output | wc -l`;\
	if(( lines != 1 )); then \
        echo "one conflict and only one conflict expected, but there are $$lines conflicts in the rules";\
        exit 1;\
    fi; \
    stateNo=`grep 'State [[:digit:]]\+ conflicts' stscript.output|awk '{print $$2}'`;\
    if (( stateNo > 0 )); then \
        sed -i "s/define CONFLICT_STATE .*/define CONFLICT_STATE $$stateNo/" parsrctx.h; \
    fi

