
# 添加模块路径（自动添加头文件路径和源文件）
# 用法: $(eval $(call add_module,路径1 路径2 ...))
define add_module
$(foreach dir,$(1),\
    $(if $(wildcard $(dir)),,\
        $(warning Directory $(dir) does not exist)\
    )\
)
CFLAGS += $(foreach dir,$(1),-I$(dir))
LOCAL_C_SRC += $(foreach dir,$(1),$(shell find "$(abspath $(dir))" -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.S" \)))
endef

# 添加纯头文件路径（不搜索源文件）
# 用法: $(eval $(call add_include_path,路径1 路径2 ...))
define add_include_path
CFLAGS += $(foreach dir,$(1),-I$(dir))
endef

# 添加静态库路径
# 用法: $(eval $(call add_library_path,路径1 路径2 ...))
define add_library_path
LDFLAGS += $(foreach dir,$(1),-L$(dir))
endef

# 增强版源文件查找函数
# 参数1: 要搜索的路径
# 参数2(可选): 要排除的路径
define find_sources
$(if $(wildcard $(1)), \
    $(shell find "$(abspath $(1))" -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.S" \) $(if $(2),-not -path "$(abspath $(2))/*")), \
    $(warning Directory $(1) does not exist) \
)
endef

# 多路径源文件收集
# 用法: $(call find_sources_multiple,路径1 路径2 ...)
define find_sources_multiple
$(foreach dir,$(1),$(call find_sources,$(dir)))
endef
