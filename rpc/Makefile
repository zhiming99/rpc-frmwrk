include ../Makefile.init.pub
C_INCLUDE += -I../ipc

TARGET:=librpc.so
SOURCES:= dbusprxy.cpp prxyfdo.cpp fdodrv.cpp \
	tcpport.cpp stmpdo.cpp tcpfido.cpp reqfwdr.cpp \
	rpcroute.cpp bridge.cpp filetran.cpp dllexp.cpp \
	senddata.cpp

OBJS:=$(SOURCES:%.cpp=$(OBJ_DIR)/%.o)

all: $(OUTPUT_DIR)/$(TARGET)
debug: $(OUTPUT_DIR)/$(TARGET)
release: $(OUTPUT_DIR)/$(TARGET)

$(OUTPUT_DIR)/$(TARGET) : $(OBJS)
	@if [ ! -d $(OUTPUT_DIR) ]; then mkdir -p $(OUTPUT_DIR);fi
	$(CC) -o $(OUTPUT_DIR)/$(TARGET) $(LINKER_OPTION) $(OBJS)

-include $(OBJS:%.o=%.d)

debug_clean: do_clean
release_clean: do_clean

clean:
	make debug_clean
	make release_clean

do_clean:
	-@rm $(OBJ_DIR)/*.d $(MUTE_CLEAN)
	-@rm $(OUTPUT_DIR)/$(TARGET) $(MUTE_CLEAN)
	-@rm $(OBJS) $(MUTE_CLEAN)

sync:
	bash ./sync-up.sh

.PHONY: sync
