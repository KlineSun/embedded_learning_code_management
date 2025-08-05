
-include $(MY_PROJECT_ROOT)/Makefile

OBJDUMP := objdump
LOCAL_DIR := $(shell pwd)

LOCAL_C_SRC := $(shell find "$(LOCAL_DIR)" -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.S" \))

# include common source files
LOCAL_C_SRC += $(COMMON_SRC_FILES)

# include utils src and include path
UTILS_DIR_PATH := $(UTILS_ROOT_PATH)/screen
CFLAGS += $(foreach dir,$(UTILS_DIR_PATH), -I $(dir))
LOCAL_C_SRC += $(foreach dir,$(UTILS_DIR_PATH),$(shell find "$(dir)" -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.S" \)))
# LOCAL_C_SRC += $(shell find "$(UTILS_ROOT_PATH)/i2c" -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.S" \))

CFLAGS += -lfreetype -lm
LOCAL_TARGET := oled_fb_tst
LOCAL_BUILD_TYPE := ARM_BUILD

# define dependency
LOCAL_DEP_OBJECTS := $(patsubst %.c,%.o,$(filter %.c,$(LOCAL_C_SRC))) \
                    $(patsubst %.cpp,%.o,$(filter %.cpp,$(LOCAL_C_SRC))) \
                    $(patsubst %.S,%.o,$(filter %.S,$(LOCAL_C_SRC)))

ifeq ($(LOCAL_BUILD_TYPE), ARM_BUILD)
	CC := $(ARM_BUILD_PREFIX)$(CC)
	AR := $(ARM_BUILD_PREFIX)$(AR)
	RANLIB := $(ARM_BUILD_PREFIX)$(RANLIB)
	OBJDUMP := $(ARM_BUILD_PREFIX)$(OBJDUMP)
endif

$(LOCAL_TARGET): $(LOCAL_DEP_OBJECTS)
	@echo "include common source files: $(COMMON_SRC_FILES)"
	@echo "depends: $(LOCAL_DEP_OBJECTS)"
	@echo "making $(LOCAL_TARGET)"
	$(CC) $(CFLAGS) -o $(LOCAL_DIR)/$(LOCAL_TARGET) $^

# C 文件
%.o: %.c $(PRE_BUILD_TARGET)
	@echo "compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

# C++ 文件
%.o: %.cpp $(PRE_BUILD_TARGET)
	@echo "compiling $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 汇编文件
%.o: %.S $(PRE_BUILD_TARGET)
	@echo "assembling $<"
	$(CC) $(ASFLAGS) -c $< -o $@

$(PRE_BUILD_TARGET):
	@if [ ! -d $(RESOURCE_TARGET_DIR) ]; then \
		@echo "Directory $(RESOURCE_TARGET_DIR) does not exist. Creating..."; \
		mkdir -p $(RESOURCE_TARGET_DIR); \
	fi
	touch $(PRE_BUILD_TARGET)

.PHONY: clean dis
clean:
	rm -f $(LOCAL_DIR)/$(LOCAL_TARGET) $(LOCAL_DEP_OBJECTS) $(PRE_BUILD_TARGET)

dis:
	$(OBJDUMP) -D $(LOCAL_TARGET) > $(LOCAL_TARGET).dis
