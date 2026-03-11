ifndef TARGET_COMPILE
    TARGET_COMPILE = ../../arm-gnu-toolchain-15.2.rel1-mingw-w64-x86_64-aarch64-none-elf/bin/aarch64-none-elf-
endif

ifndef KP_DIR
    KP_DIR = ../..
endif

CC = $(TARGET_COMPILE)gcc
LD = $(TARGET_COMPILE)ld

INCLUDE_DIRS := . include patch/include linux/include linux/arch/arm64/include linux/tools/arch/arm64/include
INCLUDE_FLAGS := $(foreach dir,$(INCLUDE_DIRS),-I$(KP_DIR)/kernel/$(dir))

objs := wxshadow_lite.o

all: wxshadow_lite.kpm

wxshadow_lite.kpm: ${objs}
	${CC} -r -o $@ $^

%.o: %.c
	${CC} $(CFLAGS) $(INCLUDE_FLAGS) -Twxshadow_lite.lds -c -O2 -o $@ $<

.PHONY: clean
clean:
	rm -rf *.kpm
	find . -name "*.o" | xargs rm -f
