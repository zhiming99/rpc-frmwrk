include ../Makefile.init.pub

TARGET:=libcombase.so
OBJS:= clsids.o autoptr.o buffer.o \
	config.o objbase.o \
	registry.o \
	creatins.o \
	serial.o \
	dbuserr.o \

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

clsids.o : clsids.cpp
autoptr.o : autoptr.cpp
buffer.o : buffer.cpp
config.o : config.cpp
objbase.o : objbase.cpp
registry.o : registry.cpp
creatins.o : creatins.cpp
serial.o : serial.cpp
dbuserr.o : dbuserr.cpp

clean:
	@rm *.o *.d
	@rm $(OUTPUT_DIR)/$(TARGET)

.PHONY: sync
