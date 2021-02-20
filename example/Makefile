src = $(wildcard *.c)
obj = $(src:.c=.o)
TOOLCHAIN = ..
CC = powerpc-linux-gnu-gcc
CFLAGS = -nostdlib -static -I $(TOOLCHAIN)/include -O1 -fno-builtin -Werror -Wall -Wno-unused-variable

all: tbrc_patched.elf

hooks.elf: $(obj)
	powerpc-linux-gnu-ld -o $@ $^ -T $(TOOLCHAIN)/linker/link.x

tbrc_patched.elf: hooks.elf hooks.txt
	$(TOOLCHAIN)/elfpatch ~/tbrc_ram.elf hooks.elf hooks.txt tbrc_patched.elf


.PHONY: clean all rebuild
clean:
	rm -f $(obj) hooks.elf tbrc_patched.elf
rebuild: clean all
