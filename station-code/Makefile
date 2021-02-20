src = $(wildcard lib/*.c) $(wildcard src/*.c)
obj = build/gen.o $(src:%.c=build/%.o)
TOOLCHAIN = ..
CC = powerpc-linux-gnu-gcc
LD = powerpc-linux-gnu-ld
CFLAGS = -c -nostdlib -static -I include -O1 -fno-builtin -Werror -Wall -Wno-unused-variable

all: tbrc_patched.elf brc.code.1

hooks.elf: $(obj)
	$(LD) -o $@ $^ -T linker/link.x

# tbrc_ram.elf by default is a link to the project binaries-and-firmware
tbrc_patched.elf: hooks.elf hooks.txt
	$(TOOLCHAIN)/elfpatch tbrc_ram.elf hooks.elf hooks.txt tbrc_patched.elf

brc.code.1: tbrc_patched.elf
	gzip -c tbrc_patched.elf > brc.code.1

.PHONY: clean all rebuild
clean:
	rm -f $(obj) hooks.elf tbrc_patched.elf
rebuild: clean all

build/lib/%.o: lib/%.c
	@mkdir -p build/lib
	$(CC) $(CFLAGS) -o $@ $<

build/src/%.o: src/%.c
	@mkdir -p build/src
	$(CC) $(CFLAGS) -o $@ $<

build/gen.o: $(src)
	@mkdir -p build
	lib/gencode.sh $(src) > build/gen.c
	$(CC) $(CFLAGS) -o $@ build/gen.c

depend: .depend
.depend: $(src)
	@rm -f ./.depend
	$(CC) $(CFLAGS) -MM $^ -MF  ./.depend;

include .depend
