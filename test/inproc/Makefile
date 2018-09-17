#!make
include ../Makefile.init.pub

TARGET_SVR:=inproctst

TARGET:=$(TARGET_SVR)
OBJS_SVR:= inproctst.o

InProcLib:=libinproc.so
OBJS2:= inprocsvr.o inprocli.o

LIB_DIR:=$(lastword $(OUTPUT_DIR) )

LINKER_OPTION += -ldl -lstdc++ -L$(BASE)/debug/ -lipc -lcombase -L$(BASE)/test/debug/ 

all: $(TARGET)
debug: $(TARGET)
release: $(TARGET)

-include $(OBJS2:.o=.d)
-include $(OBJS_SVR:.o=.d)

$(TARGET_SVR) : $(InProcLib) $(OBJS_SVR)
	@if [ ! -d $(OUTPUT_DIR) ]; then mkdir -p $(OUTPUT_DIR);fi
	$(CC) -o $(OUTPUT_DIR)/$(TARGET_SVR) $(LINKER_OPTION) -linproc $(OBJS_SVR)

$(InProcLib) : $(OBJS2)
	@if [ ! -d $(OUTPUT_DIR) ]; then mkdir -p $(OUTPUT_DIR);fi
	$(CC) -o $(OUTPUT_DIR)/$(InProcLib) $(LINKER_OPTION) -shared $(OBJS2)

%.o : %.cpp
	@echo Building file: $<
	$(CPP) $(C_DEFINES) $(C_FLAGS) $(CPP_FLAGS) $(C_INCLUDE) -o $@ $(CUR_DIR)/$<
	@echo
	@$(CPP) $(C_DEFINES) $(C_FLAGS ) $(CPP_FLAGS) $(C_INCLUDE) -MM $*.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

inproctst.o : inproctst.cpp
	@echo Building file: $<
	$(CPP) $(C_DEFINES) $(C_FLAGS) $(CPP_FLAGS) $(C_INCLUDE) -o $@ $(CUR_DIR)/$<
	@echo
	@$(CPP) $(C_DEFINES) $(C_FLAGS ) $(CPP_FLAGS) $(C_INCLUDE) -MM inproctst.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

inprocli.o : inprocli.cpp
inprocsvr.o : inprocsvr.cpp 


clean:
	-@rm *.o *.d
	-@rm $(OUTPUT_DIR)/$(TARGET_SVR)
	-@rm $(OUTPUT_DIR)/$(InProcLib)

.PHONY: sync TARGET
