
-include $(MY_PROJECT_ROOT)/Makefile

LOCAL_DIR := $(shell pwd)

LOCAL_C_SRC := $(shell find "$(LOCAL_DIR)" -type f \( -name "*.c" -o -name "*.cpp" \))

# include common source files
LOCAL_C_SRC += $(COMMON_SRC_FILES)

# include utils src and include path
LOCAL_C_SRC += $(shell find "$(UTILS_ROOT_PATH)/i2c" -type f \( -name "*.c" -o -name "*.cpp" \))
COMMON_I2C_INCLUDE_PATH := $(shell find $(UTILS_ROOT_PATH)/i2c -type d)
CFLAGS += $(foreach dir,$(COMMON_I2C_INCLUDE_PATH), -I $(dir))

CFLAGS += -lpthread -li2c -lm
LOCAL_TARGET := i2c_tst
LOCAL_BUILD_TYPE := ARM_BUILD

# define dependency
LOCAL_DEP_OBJECTS := $(patsubst %.c,%.o,$(LOCAL_C_SRC))

ifeq ($(LOCAL_BUILD_TYPE), ARM_BUILD)
	CC := $(ARM_BUILD_PREFIX)$(CC)
	AR := $(ARM_BUILD_PREFIX)$(AR)
	RANLIB := $(ARM_BUILD_PREFIX)$(RANLIB)
endif

$(LOCAL_TARGET): $(LOCAL_DEP_OBJECTS)
	@echo "include common source files: $(COMMON_SRC_FILES)"
	@echo "depends: $(LOCAL_DEP_OBJECTS)"
	@echo "making $(LOCAL_TARGET)"
	$(CC) $(CFLAGS) -o $(LOCAL_DIR)/$(LOCAL_TARGET) $^

%.o: %.c $(PRE_BUILD_TARGET)
	@echo "compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@;

$(PRE_BUILD_TARGET):
	@if [ ! -d $(RESOURCE_TARGET_DIR) ]; then \
		@echo "Directory $(RESOURCE_TARGET_DIR) does not exist. Creating..."; \
		mkdir -p $(RESOURCE_TARGET_DIR); \
	fi
	touch $(PRE_BUILD_TARGET)

.PHONY: clean
clean:
	rm -f $(LOCAL_DIR)/$(LOCAL_TARGET) $(LOCAL_DEP_OBJECTS) $(PRE_BUILD_TARGET)
