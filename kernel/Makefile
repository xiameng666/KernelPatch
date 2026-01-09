# KernelPatch内核模块构建脚本

# 检查交叉编译工具链是否设置
ifndef TARGET_COMPILE
    $(error TARGET_COMPILE not set)
endif

# 目标文件名
TARGET=kpimg

# 交叉编译工具链定义
CC = $(TARGET_COMPILE)gcc
LD = $(TARGET_COMPILE)ld
AS = $(TARGET_COMPILE)as
OBJCOPY = $(TARGET_COMPILE)objcopy

# 编译选项：禁用内建函数、指定C标准、禁用标准头文件
CFLAGS += -Wall -fno-builtin -std=gnu11 -nostdinc
CFLAGS += -g

# 调试模式编译选项
ifdef DEBUG
	CFLAGS += -DDEBUG -DMAP_DEBUG -g
endif

# Android平台编译选项
ifdef ANDROID
	CFLAGS += -DANDROID
endif

# 头文件包含路径
INCLUDE := -I. -Iinclude -Ipatch/include -Ilinux -Ilinux/include -Ilinux/arch/arm64/include -Ilinux/tools/arch/arm64/include

# 基础源文件列表
BASE_SRCS += base/setup.c 
BASE_SRCS += base/setup1.S
BASE_SRCS += base/cache.S
BASE_SRCS += base/tlsf.c
BASE_SRCS += base/start.c 
BASE_SRCS += base/map.c 
BASE_SRCS += base/map1.S 
BASE_SRCS += base/hook.c 
BASE_SRCS += base/fphook.c 
BASE_SRCS += base/hmem.c 
BASE_SRCS += base/predata.c 
BASE_SRCS += base/symbol.c 
BASE_SRCS += base/baselib.c 
BASE_SRCS += base/sha256.c 

# 添加补丁相关源文件
BASE_SRCS += $(wildcard patch/*.c)
BASE_SRCS += $(wildcard patch/common/*.c)
BASE_SRCS += $(wildcard patch/module/*.c)
BASE_SRCS += $(wildcard patch/ksyms/*.c)

# Android平台特定源文件
ifdef ANDROID
	BASE_SRCS += $(wildcard patch/android/*.c)
endif

# 所有源文件
SRCS += $(BASE_SRCS)
SRCS += $(LINUX_SRCS)

# 目标文件列表（将.c和.S替换为.o）
OBJS := $(SRCS:.c=.o)
OBJS := $(OBJS:.S=.o)

# 默认构建目标
all: hdr ${TARGET}

# 生成二进制镜像文件
${TARGET}: ${TARGET}.elf
	${OBJCOPY} -O binary -S $^ $@

# 链接生成ELF文件
${TARGET}.elf: ${OBJS}
	${LD} -nostdlib -static -no-pie -Tkpimg.lds -o $@ $^

# C源文件编译规则
%.o: %.c
	${CC} $(CFLAGS) $(INCLUDE) -c -O2 -o $@ $<

# 汇编源文件编译规则
%.o: %.S
	${CC} $(CFLAGS) $(INCLUDE) -c -o $@ $<

# 生成头文件拷贝目标
.PHONY: hdr
hdr:
	cp -Rf patch/include/uapi ../user/
	cp -f ../version ../user/
	cp -f include/preset.h ../tools/

# 清理构建文件
.PHONY: clean
clean:
	rm -rf *.elf
	rm -rf kpimg
	find . -name *.o | xargs rm -f