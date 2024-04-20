# 根目录mk文件，定义全局的编译变量
CC := gcc
AR := ar
RANLIB := ranlib

export CC AR RANLIB

# 工程根目录
#MY_PROJECT_ROOT := 

# 定义工程中用到的公共的静态库、动态库和头文件目录
COMMON_LIB_PATH	   := $(MY_PROJECT_ROOT)/lib/share/lib $(MY_PROJECT_ROOT)/lib/static/lib
COMMON_INCLUE_PATH := $(MY_PROJECT_ROOT)/inc $(MY_PROJECT_ROOT)/common

# 编译标志
CFLAGS := -Wall -O2 -g
CFLAGS += $(foreach dir,$(COMMON_LIB_PATH),-L $(dir))
CFLAGS += $(foreach dir,$(COMMON_INCLUE_PATH),-I $(dir))

COMMON_SRC_FILES := $(wildcard $(MY_PROJECT_ROOT)/common/*.c)
COMMON_DEP_OBJS  := $(patsubst %.c,%.o,$(COMMON_SRC_FILES))
# 链接标志
LDFLAGS := -lm

# 导出公共的编译变量，以便子目录下用到
export MY_PROJECT_ROOT  COMMON_SHARE_LIB_PATH COMMON_STATIC_LIB_PATH COMMON_INCLUE_PATH CFLAGS LDFLAGS
