
#ifndef ELFPATCHER_VIRTUAL_H
#define ELFPATCHER_VIRTUAL_H
#include <unistd.h>
#include <elf.h>
#include <gelf.h>

uint32_t readVirtualAddress(Elf *elf, uint32_t addr);
void writeVirtualAddress(Elf *elf, uint32_t addr, uint32_t val);

#endif //ELFPATCHER_VIRTUAL_H
