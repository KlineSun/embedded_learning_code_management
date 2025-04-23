

# Makefile知识进阶

## 一、基础语法补充

- **strip命令：**
  `strip` 命令会删除以下信息，以减小库的大小：

  - 如 `-g` 编译生成的 DWARF 信息
  - 未使用的符号表、重定位信息等

  效果：

  - 对动态库，文件体积可减少 **50%~90%**（例如从 2MB 降到 200KB）
  - **不影响库的功能**，但会失去调试能力（如 `gdb` 无法显示源码行号）
  - 对静态库，`strip` 会直接处理整个 `.a` 文件（通常效果有限）；
  - 更彻底的做法是先用 `ar x` 解包，剥离每个 `.o` 文件后重新打包（此 Makefile 未采用）。

- **install命令**
   Linux/Unix 系统中用于**文件安装的标准工具**
  **基本语法**：install [选项] 源文件 目标路径
  **常用选项**：

  - `-m <权限>`：设置文件权限（如 `-m 755` 表示 `rwxr-xr-x`）
  - `-d`：创建目录（类似 `mkdir -p`）
  - `-s`：剥离调试符号（类似 `strip`）

  **使用示例**：

  - 创建目录
    install -m 755 -d /usr/local/include/i2c
  - 安装头文件
    install -m 644 include/i2c/smbus.h /usr/local/include/i2c/smbus.h
  - 安装动态库
    install -m 755 lib/libi2c.so.0.1.1 /usr/local/lib
  - 安装静态库
    install -m 644 lib/libi2c.a /usr/local/lib
  - 安装手册页
    install -m 644 lib/libi2c.3 /usr/local/share/man/man3

  **为什么使用install而不是cp？**

| 特性             | `install`                  | `cp`                 |
| :--------------- | :------------------------- | :------------------- |
| **权限控制**     | 可直接设置目标权限（`-m`） | 需额外调用 `chmod`   |
| **目录创建**     | 自动创建缺失目录（`-d`）   | 需手动 `mkdir -p`    |
| **符号链接处理** | 默认复制链接指向的文件内容 | 可通过 `-P` 保留链接 |
| **调试符号剥离** | 支持 `-s` 选项自动剥离     | 需手动调用 `strip`   |
| **原子性操作**   | 更安全（避免部分写入）     | 直接覆盖             |

- 
  **关键注意事项**
  - 权限设置
    动态库需 `755`（需可执行），静态库和头文件用 `644`。
  - 目录创建顺序：
    先`install -d` 创建目录，再安装文件。
  - 安装路径
    使用 `$(DESTDIR)` 支持打包系统（如 `make DESTDIR=/tmp/pkg install`）。
- **链接器命名规则**：
  - `-l` 参数后应跟**库的短名**（去掉 `lib` 前缀和 `.so` 后缀）。
  - 链接器会自动查找最新兼容版本（如 `libi2c.so.0` 或 `libi2c.so.0.1.1`）。
- **版本号处理**：
  - 动态库的版本号（`.so.0.1.1`）由 SONAME 管理，编译时只需指定主版本（如 `-li2c`），运行时加载器会根据 SONAME 找具体文件





## 二、多级makefile构建

### 1、makefile结构设计

主makefile/根目录makefile：

- 定义工程全局使用的变量，如编译器等：

  CC    = $(CROSS_COMPILE)gcc

  AR    = $(CROSS_COMPILE)ar

  STRIP  = $(CROSS_COMPILE)strip

- 定义伪目标：
  .PHONY: all strip clean install uninstall

- 定义第一个目标，可供在子目录下的makefile文件中补充：

  all:

- 包含子目录下的makefile文件：
  SRCDIRS := include lib eeprom stub tools $(EXTRA)

  include $(SRCDIRS:%=%/Module.mk)



子目录makefile：

- 定义子目录的构建路径，例如：
  LIB_DIR   := lib

  LIB_CFLAGS  += -Wstrict-prototypes -Wshadow -Wpointer-arith -Wcast-qual \

  ​      -Wcast-align -Wwrite-strings -Wnested-externs -Winline \

  ​      -W -Wundef -Wmissing-prototypes -Iinclude

- 定义子目录下的目标：
  LIB_TARGETS :=

  ifeq ($(BUILD_DYNAMIC_LIB),1)

  LIB_LINKS  := $(LIB_SHSONAME) $(LIB_SHBASENAME)

  LIB_TARGETS += $(LIB_SHLIBNAME)

  endif

  ifeq ($(BUILD_STATIC_LIB),1)

  LIB_TARGETS += $(LIB_STLIBNAME)

  endif

- 定义子目录下的依赖：
  ifeq ($(USE_STATIC_LIB),1)

  LIB_DEPS   := $(LIB_DIR)/$(LIB_STLIBNAME)

  else

  LIB_DEPS   := $(LIB_DIR)/$(LIB_SHBASENAME)

  endif

- 补充在根目录下定义的目标：

  all-lib: $(addprefix $(LIB_DIR)/,$(LIB_TARGETS) $(LIB_LINKS))

  strip-lib: $(addprefix $(LIB_DIR)/,$(LIB_TARGETS))

    $(STRIP) $(addprefix $(LIB_DIR)/,$(LIB_TARGETS))
  

  all: all-lib

  strip: strip-lib





