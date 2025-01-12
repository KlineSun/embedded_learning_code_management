
-include $(MY_PROJECT_ROOT)/Makefile

LOCAL_DIR := $(shell pwd)

LOCAL_C_SRC := $(wildcard *.c) $(wildcard */*.c)
LOCAL_DEP_OBJECTS := $(addprefix $(LOCAL_DIR)/, $(LOCAL_C_SRC:.c=.o))

# include common source files
LOCAL_C_SRC += $(COMMON_SRC_FILES)
LOCAL_DEP_OBJECTS += $(COMMON_DEP_OBJS)

CFLAGS += -lfreetype -lm
LOCAL_TARGET := freetype_tst_vec
LOCAL_BUILD_TYPE := ARM_BUILD

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
	cp -r $(RESOURCE_SOUCE_FILES) $(RESOURCE_TARGET_DIR)
	touch $(PRE_BUILD_TARGET)

.PHONY: clean
clean:
	rm -f $(LOCAL_DIR)/$(LOCAL_TARGET) $(LOCAL_DEP_OBJECTS) $(PRE_BUILD_TARGET)
