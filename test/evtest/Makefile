#!make
include ../Makefile.init.pub

TARGET_SVR:=evtsvrtst
TARGET_CLI:=evtclitst

TARGET:=$(TARGET_SVR) $(TARGET_CLI)
OBJS_SVR:= evtsvrtst.o
OBJS_CLI:= evtclitst.o

EvtTestLib:=libevtst.so
OBJS2:= evtsvr.o evtcli.o

LIB_DIR:=$(lastword $(OUTPUT_DIR) )

LINKER_OPTION += -ldl -lstdc++ -L$(BASE)/debug/ -lipc -lcombase -L$(BASE)/test/debug/ 

all: $(TARGET)
debug: $(TARGET)
release: $(TARGET)

-include $(OBJS2:.o=.d)
-include $(OBJS_SVR:.o=.d)
-include $(OBJS_CLI:.o=.d)

$(TARGET_SVR) : $(EvtTestLib) $(OBJS_SVR)
	@if [ ! -d $(OUTPUT_DIR) ]; then mkdir -p $(OUTPUT_DIR);fi
	$(CC) -o $(OUTPUT_DIR)/$(TARGET_SVR) $(LINKER_OPTION) -levtst $(OBJS_SVR)

$(TARGET_CLI) : $(EvtTestLib) $(OBJS_CLI)
	@if [ ! -d $(OUTPUT_DIR) ]; then mkdir -p $(OUTPUT_DIR);fi
	$(CC) -o $(OUTPUT_DIR)/$(TARGET_CLI) $(LINKER_OPTION) -levtst $(OBJS_CLI)

$(EvtTestLib) : $(OBJS2)
	@if [ ! -d $(OUTPUT_DIR) ]; then mkdir -p $(OUTPUT_DIR);fi
	$(CC) -o $(OUTPUT_DIR)/$(EvtTestLib) $(LINKER_OPTION) -shared $(OBJS2)

%.o : %.cpp
	@echo Building file: $<
	$(CPP) $(C_DEFINES) $(C_FLAGS) $(CPP_FLAGS) $(C_INCLUDE) -o $@ $(CUR_DIR)/$<
	@echo
	@$(CPP) $(C_DEFINES) $(C_FLAGS ) $(CPP_FLAGS) $(C_INCLUDE) -MM $*.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

evtsvrtst.o : evtest.cpp
	@echo Building file: $<
	$(CPP) $(C_DEFINES) $(C_FLAGS) $(CPP_FLAGS) $(C_INCLUDE) -DSERVER -o $@ $(CUR_DIR)/$<
	@echo
	@$(CPP) $(C_DEFINES) $(C_FLAGS ) $(CPP_FLAGS) $(C_INCLUDE) -MM evtest.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

evtclitst.o : evtest.cpp
	@echo Building file: $<
	$(CPP) $(C_DEFINES) $(C_FLAGS) $(CPP_FLAGS) $(C_INCLUDE) -DCLIENT -o $@ $(CUR_DIR)/$<
	@echo
	@$(CPP) $(C_DEFINES) $(C_FLAGS ) $(CPP_FLAGS) $(C_INCLUDE) -MM evtest.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

evtcli.o : evtcli.cpp
evtsvr.o : evtsvr.cpp 


clean:
	-@rm *.o *.d
	-@rm $(OUTPUT_DIR)/$(TARGET_SVR)
	-@rm $(OUTPUT_DIR)/$(TARGET_CLI)
	-@rm $(OUTPUT_DIR)/$(EvtTestLib)

.PHONY: sync TARGET
