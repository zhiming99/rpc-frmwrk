# Makefile for ANTLR4 C++ Parser

CXX = g++
ANTLR4_DIR = /home/zhiming/mywork/antlr4-4.13.2
jsoncpp=$(shell pkg-config --cflags jsoncpp)
CXXFLAGS = -std=c++17 -Wall -I. -I../../../include -I$(ANTLR4_DIR)/runtime/Cpp/run/usr/local/include/antlr4-runtime -O0 -ggdb -Wno-overloaded-virtual $(jsoncpp)
LDFLAGS = -L$(ANTLR4_DIR)/runtime/Cpp/run/usr/local/lib -lantlr4-runtime -Wl,-rpath,$(ANTLR4_DIR)/runtime/Cpp/run/usr/local/lib

# ANTLR tool jar (4.13.2)
ANTLR4_JAR = $(ANTLR4_DIR)/antlr4-tool.jar

# Source files
LEXER = stlexer
PARSER = stparser

# Executable
TARGET = $(rpcfstc)

# Additional executables
rpcfstc = rpcfstc

# Default target
all: $(TARGET)

other_obj=stparserVisitor.o stparserBaseListener.o stparserListener.o stparserBaseVisitor.o
gen_src=stparserVisitor.cpp stparserBaseListener.cpp stparserListener.cpp stparserBaseVisitor.cpp stparser.cpp stlexer.cpp \
stparserVisitor.h stparserBaseListener.h stparserListener.h stparserBaseVisitor.h stparser.h stlexer.h

# Build rpcfstc
$(rpcfstc): stlexer.o stparser.o st_main.o st_parse_listener.o st_parser_ext.o $(other_obj)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile rpcfstc
rpcfstc.o: st_main.cpp parse_context.h st_parse_listener.h st_parser_ext.h
	$(CXX) $(CXXFLAGS) -c rpcfstc.cpp -o rpcfstc.o

# Compile lexer
stlexer.o: stlexer.cpp stlexer.h
	$(CXX) $(CXXFLAGS) -c stlexer.cpp -o stlexer.o

# Compile parser
stparser.o: stparser.cpp stparser.h 
	$(CXX) $(CXXFLAGS) -c stparser.cpp -o stparser.o


# Compile parse listener
st_parse_listener.o: st_parse_listener.cpp st_parse_listener.h stparser.h parse_context.h
	$(CXX) $(CXXFLAGS) -c st_parse_listener.cpp -o st_parse_listener.o

# Regenerate parser from grammar

stlexer.cpp stlexer.h stparser.cpp stparser.h stparserVisitor.h stparserBaseListener.h stparserListener.h stparserBaseVisitor.h: ../stparser.g4 ../stlexer.g4
	cd .. && java -jar $(ANTLR4_JAR) -Dlanguage=Cpp -visitor -listener -o ./topdown_cpp stparser.g4 stlexer.g4

# Clean
clean:
	rm -f *.o $(rpcfstc) *.tokens $(gen_src)

# Phony targets
.PHONY: all regen clean 
