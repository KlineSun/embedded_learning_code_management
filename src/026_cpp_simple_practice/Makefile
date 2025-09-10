
-include $(MY_PROJECT_ROOT)/Makefile
include $(MY_PROJECT_ROOT)/build/func.mk

LOCAL_DIR := $(shell pwd)
LOCAL_TARGET := cpp_quick_practice
# LOCAL_BUILD_TYPE := ARM_BUILD
CFLAGS += -lm

SRC_DIR_PATH := $(LOCAL_DIR)

$(eval $(call add_module, $(SRC_DIR_PATH)))

include $(MY_PROJECT_ROOT)/build/base_rule.build