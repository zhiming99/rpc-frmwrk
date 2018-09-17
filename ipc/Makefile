include ../Makefile.init.pub

TARGET:=libipc.so
OBJS:= ifstat.o rpcif.o \
	mainloop.o dllexp.o\
	irp.o irpthread.o port.o portstat.o \
	drvmgr.o utils.o iomgr.o ptifmap.o \
	pnpmgr.o dbusdrv.o dbusport.o portdrv.o \
	rpcpdo.o dbuspdo.o tasklets.o \
	iftasks.o lpbkmtch.o ifhelper.o \
	iiddict.o reqopen.o

all: $(TARGET)
debug: $(TARGET)
release: $(TARGET)

$(TARGET) : $(OBJS)
	@if [ ! -d $(OUTPUT_DIR) ]; then mkdir -p $(OUTPUT_DIR);fi
	$(CC) -o $(OUTPUT_DIR)/$(TARGET) $(LINKER_OPTION) $(OBJS)

-include $(OBJS:.o=.d)

%.o : %.cpp
	@echo Building file: $<
	$(CPP) $(C_DEFINES) $(C_FLAGS ) $(CPP_FLAGS) $(C_INCLUDE) -o $@ $(CUR_DIR)/$<
	@echo
	@$(CPP) $(C_DEFINES) $(C_FLAGS ) $(CPP_FLAGS) $(C_INCLUDE) -MM $*.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

ifhelper.o : ifhelper.cpp
dllexp.o : dllexp.cpp
mainloop.o : mainloop.cpp
irp.o : irp.cpp
irpthread.o : irpthread.cpp
port.o	: port.cpp
portstat.o	: portstat.cpp
drvmgr.o : drvmgr.cpp
utils.o : utils.cpp
iomgr.o : iomgr.cpp
ptifmap.o : ptifmap.cpp
pnpmgr.o : pnpmgr.cpp
dbusdrv.o : dbusdrv.cpp
dbusport.o : dbusport.cpp
portdrv.o : portdrv.cpp
rpcpdo.o : rpcpdo.cpp
dbuspdo.o : dbuspdo.cpp
tasklets.o : tasklets.cpp
ifstat.o : ifstat.cpp
rpcif.o	: rpcif.cpp
iftasks.o : iftasks.cpp
lpbkmtch.o : lpbkmtch.cpp
iiddict.o : iiddict.cpp
reqopen.o : reqopen.cpp

clean:
	@rm *.o *.d
	@rm $(OUTPUT_DIR)/$(TARGET)

sync:
	bash ./sync-up.sh


.PHONY: sync
