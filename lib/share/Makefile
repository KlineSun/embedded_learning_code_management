
-include $(MY_PROJECT_ROOT)/Makefile

LOCAL_DIR := $(shell pwd)

LOCAL_C_SRC := $(wildcard *.c) $(wildcard */*.c)

LIB_DEP_OBJECTS := $(addprefix $(LOCAL_DIR)/, $(LOCAL_C_SRC:.c=.o))

LOCAL_TARGET := my_share_lib.so

$(LOCAL_TARGET): $(LIB_DEP_OBJECTS)
	@echo "making lib"
	$(CC) $(CFLAGS) -shared -o $(LOCAL_DIR)/$(LOCAL_TARGET) $^
	mv $(LOCAL_DIR)/$(LOCAL_TARGET) $(LOCAL_DIR)/lib

%.o: %.c
	@echo "compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(LOCAL_DIR)/lib/$(LOCAL_TARGET).so $(LIB_DEP_OBJECTS)
	