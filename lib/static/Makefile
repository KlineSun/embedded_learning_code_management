
-include $(MY_PROJECT_ROOT)/Makefile

LOCAL_DIR := $(shell pwd)

LOCAL_C_SRC := $(wildcard *.c) $(wildcard */*.c)

LIB_DEP_OBJECTS := $(addprefix $(LOCAL_DIR)/, $(LOCAL_C_SRC:.c=.o))

LOCAL_TARGET := my_static_lib

default: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LIB_DEP_OBJECTS)
	@echo "making lib"
	$(AR) rcs $(LOCAL_DIR)/$(LOCAL_TARGET).a $
	mv $(LOCAL_DIR)/$(LOCAL_TARGET).a $(LOCAL_DIR)/lib

%.o: %.c
	@echo "compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean $(LOCAL_TARGET)
clean:
	rm -f $(LOCAL_DIR)/lib/$(LOCAL_TARGET).a $(LIB_DEP_OBJECTS)
	