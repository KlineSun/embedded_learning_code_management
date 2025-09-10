# 根目录mk文件，定义全局的编译变量
CC := gcc
CXX := g++
AR := ar
RANLIB := ranlib
OBJDUMP := objdump

export CC AR RANLIB

# 定义arm build的编译变量前缀
ARM_BUILD_PREFIX := arm-buildroot-linux-gnueabihf-

# 定义工程中拥有的公共的静态库、动态库和头文件目录
COMMON_LIB_PATH	   := $(MY_PROJECT_ROOT)/lib/share/lib $(MY_PROJECT_ROOT)/lib/static/lib
COMMON_INCLUE_PATH := $(MY_PROJECT_ROOT)/inc $(MY_PROJECT_ROOT)/common/include

# 工具路径，及其下的子模块
UTILS_ROOT_PATH		:= $(MY_PROJECT_ROOT)/utils
UTIL_SCREEN_PATH   	:= $(UTILS_ROOT_PATH)/screen

# 编译标志
CFLAGS := -Wall -O2 -g
CXXFLAGS := -lstdc++
CFLAGS += $(foreach dir,$(COMMON_LIB_PATH),-L $(dir))
CFLAGS += $(foreach dir,$(COMMON_INCLUE_PATH),-I $(dir))
# $(info root CFLAGS: $(CFLAGS))

COMMON_SRC_FILES := $(shell find $(MY_PROJECT_ROOT)/common -name '*.c')
COMMON_DEP_OBJS  := $(patsubst %.c,%.o,$(COMMON_SRC_FILES))
# 链接标志
LDFLAGS :=

# 汇编标志
ASFLAGS :=

RESOURCE_TARGET_DIR := ~/resource
RESOURCE_SOUCE_FILES := $(MY_PROJECT_ROOT)/res/*
PRE_BUILD_TARGET := $(MY_PROJECT_ROOT)/pre_build_ready

# 导出公共的编译变量，以便子目录下用到
export MY_PROJECT_ROOT  COMMON_SHARE_LIB_PATH COMMON_STATIC_LIB_PATH COMMON_INCLUE_PATH CFLAGS LDFLAGS
