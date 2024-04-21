
-include $(MY_PROJECT_ROOT)/Makefile

LOCAL_DIR := $(shell pwd)

LOCAL_C_SRC := $(wildcard *.c) $(wildcard */*.c)
LOCAL_DEP_OBJECTS := $(addprefix $(LOCAL_DIR)/, $(LOCAL_C_SRC:.c=.o))

# include common source files
LOCAL_C_SRC += $(COMMON_SRC_FILES)
LOCAL_DEP_OBJECTS += $(COMMON_DEP_OBJS)

LOCAL_TARGET := kv_test

$(LOCAL_TARGET): $(LOCAL_DEP_OBJECTS)
	@echo "making $(LOCAL_TARGET)"
	$(CC) $(CFLAGS) -o $(LOCAL_DIR)/$(LOCAL_TARGET) $^

%.o: %.c
	@echo "compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(LOCAL_DIR)/$(LOCAL_TARGET) $(LOCAL_DEP_OBJECTS)
