include ../Makefile.init.pub
C_INCLUDE += -I../ipc

TARGET:=librpc.so
OBJS:= dbusprxy.o prxyfdo.o fdodrv.o \
	tcpport.o stmpdo.o tcpfido.o reqfwdr.o \
	rpcroute.o bridge.o filetran.o dllexp.o \
	senddata.o

all: $(TARGET)
debug: $(TARGET)
release: $(TARGET)

$(TARGET) : $(OBJS)
	@if [ ! -d $(OUTPUT_DIR) ]; then mkdir -p $(OUTPUT_DIR);fi
	$(CC) -o $(OUTPUT_DIR)/$(TARGET) $(LINKER_OPTION) $(OBJS)

#-include $(OBJS:.o=.d)

%.o : %.cpp
	@echo Building file: $<
	$(CPP) $(C_DEFINES) $(CPP_FLAGS) $(C_INCLUDE) -o $@ $(CUR_DIR)/$<
	@echo

#@$(CPP) $(C_DEFINES) $(CPP_FLAGS) $(C_INCLUDE) -MM $*.cpp > $*.d
#@mv -f $*.d $*.d.tmp
#@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
#@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
#@rm -f $*.d.tmp

dllexp.o	: dllexp.cpp
dbusprxy.o : dbusprxy.cpp
prxyfdo.o : prxyfdo.cpp
fdodrv.o : fdodrv.cpp
tcpport.o : tcpport.cpp
stmpdo.o : stmpdo.cpp
tcpfido.o : tcpfido.cpp
reqfwdr.o : reqfwdr.cpp
rpcroute.o : rpcroute.cpp
bridge.o : bridge.cpp
filetran.o: filetran.cpp
senddata.o: senddata.cpp

clean:
	@rm *.o *.d

sync:
	bash ./sync-up.sh

.PHONY: sync
