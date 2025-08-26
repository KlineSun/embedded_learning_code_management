
-include $(MY_PROJECT_ROOT)/Makefile
include $(MY_PROJECT_ROOT)/build/func.mk

LOCAL_DIR := $(shell pwd)
LOCAL_TARGET := usb_mouse_async_tst
LOCAL_BUILD_TYPE := ARM_BUILD
CFLAGS += -lm -lusb-1.0 -lpthread

# CFLAGS += -fsanitize=address # 检查内存泄漏

SRC_DIR_PATH := $(LOCAL_DIR)

$(eval $(call add_module, $(SRC_DIR_PATH)))

include $(MY_PROJECT_ROOT)/build/base_rule.build