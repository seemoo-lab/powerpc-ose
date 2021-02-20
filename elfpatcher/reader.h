
#ifndef ELFPATCHER_READER_H
#define ELFPATCHER_READER_H

#include <elf.h>
#include <string>
#include <map>
#include <libelf.h>

void readSymbolTable(Elf* elf, std::map<std::string, uint32_t> &table);
Elf_Scn *getSectionByName(Elf *elf, const char *name);
std::string getName(Elf *elf, size_t table, size_t offset);
extern size_t strtab; // string table

#endif //ELFPATCHER_READER_H
