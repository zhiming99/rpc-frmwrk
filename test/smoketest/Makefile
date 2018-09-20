#!make
include ../Makefile.init.pub

TARGET:=smoketest
OBJS:= main.o
LIB_DIR:=$(lastword $(OUTPUT_DIR) )

LINKER_OPTION += -ldl -lstdc++ -L$(RPCLIB_DIR) -lipc -lcombase -L$(OUTPUT_DIR)

all: $(TARGET)
debug: $(TARGET)
release: $(TARGET)

$(TARGET) : $(OBJS)
	@if [ ! -d $(OUTPUT_DIR) ]; then mkdir -p $(OUTPUT_DIR);fi
	$(CC) -o $(OUTPUT_DIR)/$(TARGET) $(LINKER_OPTION) $(OBJS)

%.o : %.cpp
	@echo Building file: $<
	$(CPP) $(C_DEFINES) $(C_FLAGS) $(CPP_FLAGS) $(C_INCLUDE) -o $@ $(CUR_DIR)/$<
	@echo

main.o : main.cpp

clean:
	-@rm *.o *.d
	-@rm $(OUTPUT_DIR)/$(TARGET)

.PHONY: sync
