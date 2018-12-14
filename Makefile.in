include ./Makefile.init.pub

OUTPUT_DIR := ./debug

ifeq ($(MAKECMDGOALS),) 
	C_FLAGS += $(DEBUG_FLAGS)
else ifeq ($(MAKECMDGOALS),debug) 
	C_FLAGS += $(DEBUG_FLAGS)
else ifeq ($(MAKECMDGOALS),all) 
	C_FLAGS += $(DEBUG_FLAGS)
endif

ifeq ($(MAKECMDGOALS),release)
	C_FLAGS += $(RELEASE_FLAGS)
	C_DEFINES += $(C_DEFINES) -DPRODUCTION 
	OUTPUT_DIR := ./release
else ifeq ($(MAKECMDGOALS),clean)
	OUTPUT_DIR := ./release
endif

ifeq ($(MODE),Release)
	OUTPUT_DIR := ./release
endif

TARGETS := $(OUTPUT_DIR)/libcombase.so $(OUTPUT_DIR)/libipc.so $(OUTPUT_DIR)/librpc.so test

all: $(TARGETS)
debug: $(TARGETS)
release: $(TARGETS)
clean: $(TARGETS)

mygoal:=$(MAKECMDGOALS)
$(OUTPUT_DIR)/libcombase.so :
	-make -C ./combase $(mygoal)
	
$(OUTPUT_DIR)/libipc.so :
	-make -C ./ipc $(mygoal)

$(OUTPUT_DIR)/librpc.so :
	-make -C ./rpc $(mygoal)

test:
	-make -C test $(mygoal)

.PHONY: $(TARGETS) test
