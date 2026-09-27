# Makefile for ANTLR4 C++ Parser

CXX = g++
AR = ar
ARFLAGS = rcs

ANTLR4_DIR = /home/zhiming/mywork/antlr4-4.13.2
rpcf_dependency=$(shell pkg-config --cflags jsoncpp dbus-1)
CXXFLAGS = -std=c++17 -Wall -I. -I../../../include -I$(ANTLR4_DIR)/runtime/Cpp/run/usr/local/include/antlr4-runtime -O0 -ggdb -Wno-overloaded-virtual $(rpcf_dependency)
LDFLAGS = -L$(ANTLR4_DIR)/runtime/Cpp/run/usr/local/lib -lantlr4-runtime -Wl,-rpath,$(ANTLR4_DIR)/runtime/Cpp/run/usr/local/lib -lcombase

# ANTLR tool jar (4.13.2)
ANTLR4_JAR = $(ANTLR4_DIR)/antlr4-tool.jar

# Executable
TARGET = $(rpcfstc)

# Additional executables
rpcfstc = rpcfstc
PRAGMA_TARGET=libpragma.a

# Default target
all: $(TARGET) $(PRAGMA_TARGET)

other_obj=stparserVisitor.o stparserBaseListener.o stparserListener.o stparserBaseVisitor.o stparser.o stlexer.o
gen_src=stparserVisitor.cpp stparserBaseListener.cpp stparserListener.cpp stparserBaseVisitor.cpp stparser.cpp stlexer.cpp \
stparserVisitor.h stparserBaseListener.h stparserListener.h stparserBaseVisitor.h stparser.h stlexer.h

pragma_autogen_name = pragma_exprBaseListener.cpp  pragma_exprLexer.cpp pragma_exprParser.cpp pragma_exprBaseVisitor.cpp pragma_exprListener.cpp  pragma_exprVisitor.cpp
pragma_header_name= pragma_exprBaseListener.h  pragma_exprBaseVisitor.h  pragma_exprLexer.h  pragma_exprListener.h  pragma_exprParser.h  pragma_exprVisitor.h
pragma_auto_gen = $(pragma_autogen_name:%=pragma_expr/%)
pragma_auto_headers = $(pragma_header_name:%=pragma_expr/%)
pragma_obj = $(pragma_auto_gen:.cpp=.o)

# Build rpcfstc
$(rpcfstc): $(other_obj) st_main.o st_parse_listener.o st_parser_ext.o astnodes.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) -L. -lpragma

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Regenerate parser from grammar
$(gen_src): ../stparser.g4 ../stlexer.g4
	cd .. && java -jar $(ANTLR4_JAR) -Dlanguage=Cpp -visitor -listener -o ./topdown_cpp stparser.g4 stlexer.g4

$(PRAGMA_TARGET): $(pragma_obj) pragma.o pragma.h
	$(AR) $(ARFLAGS) -o $@ $^

$(pragma_auto_gen) $(pragma_auto_headers) : pragma_expr.g4
	java -jar $(ANTLR4_JAR) -Dlanguage=Cpp -visitor -listener -o ./pragma_expr pragma_expr.g4

# Clean
clean:
	rm -f *.o $(rpcfstc) *.tokens $(gen_src) $(pragma_obj) $(pragma_auto_gen) $(pragma_auto_headers) pragma_expr/*.tokens

# Phony targets
.PHONY: all regen clean 
