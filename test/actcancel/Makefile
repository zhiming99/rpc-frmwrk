#!make
include ../Makefile.init.pub

TARGET_SVR:=actcsvrtst
TARGET_CLI:=actclitst

TARGET:=$(TARGET_SVR) $(TARGET_CLI)
OBJS_SVR:= actcsvrtst.o
OBJS_CLI:= actclitst.o

EchoLib:=libactc.so
OBJS2:= actcsvr.o actcli.o

LIB_DIR:=$(lastword $(OUTPUT_DIR) )

LINKER_OPTION += -ldl -lstdc++ -L$(BASE)/debug/ -lipc -lcombase -L$(BASE)/test/debug/ 

all: $(TARGET)
debug: $(TARGET)
release: $(TARGET)

-include $(OBJS2:.o=.d)

$(TARGET_SVR) : $(EchoLib) $(OBJS_SVR)
	@if [ ! -d $(OUTPUT_DIR) ]; then mkdir -p $(OUTPUT_DIR);fi
	$(CC) -o $(OUTPUT_DIR)/$(TARGET_SVR) $(LINKER_OPTION) -lactc $(OBJS_SVR)

$(TARGET_CLI) : $(EchoLib) $(OBJS_CLI)
	@if [ ! -d $(OUTPUT_DIR) ]; then mkdir -p $(OUTPUT_DIR);fi
	$(CC) -o $(OUTPUT_DIR)/$(TARGET_CLI) $(LINKER_OPTION) -lactc $(OBJS_CLI)

$(EchoLib) : $(OBJS2)
	@if [ ! -d $(OUTPUT_DIR) ]; then mkdir -p $(OUTPUT_DIR);fi
	$(CC) -o $(OUTPUT_DIR)/$(EchoLib) $(LINKER_OPTION) -shared $(OBJS2)

%.o : %.cpp
	@echo Building file: $<
	$(CPP) $(C_DEFINES) $(C_FLAGS) $(CPP_FLAGS) $(C_INCLUDE) -o $@ $(CUR_DIR)/$<
	@echo
	@$(CPP) $(C_DEFINES) $(C_FLAGS ) $(CPP_FLAGS) $(C_INCLUDE) -MM $*.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

actcsvrtst.o : actctest.cpp
	@echo Building file: $<
	$(CPP) $(C_DEFINES) $(C_FLAGS) $(CPP_FLAGS) $(C_INCLUDE) -DSERVER -o $@ $(CUR_DIR)/$<
	@echo
	@$(CPP) $(C_DEFINES) $(C_FLAGS ) $(CPP_FLAGS) $(C_INCLUDE) -MM actctest.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

actclitst.o : actctest.cpp
	@echo Building file: $<
	$(CPP) $(C_DEFINES) $(C_FLAGS) $(CPP_FLAGS) $(C_INCLUDE) -DCLIENT -o $@ $(CUR_DIR)/$<
	@echo
	@$(CPP) $(C_DEFINES) $(C_FLAGS ) $(CPP_FLAGS) $(C_INCLUDE) -MM actctest.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

actcli.o : actcli.cpp
actcsvr.o : actcsvr.cpp 


clean:
	-@rm *.o *.d
	-@rm $(OUTPUT_DIR)/$(TARGET_SVR)
	-@rm $(OUTPUT_DIR)/$(TARGET_CLI)
	-@rm $(OUTPUT_DIR)/$(EchoLib)

.PHONY: sync TARGET
