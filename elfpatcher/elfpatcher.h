#ifndef ELFPATCHER_ELFPATCHER_H
#define ELFPATCHER_ELFPATCHER_H


#include <cstdint>
#include <string>
#include <map>
#include <gelf.h>

extern Elf *dstelf;
extern std::map<uint32_t, uint32_t> hooks;
extern std::map<std::string, uint32_t> symbols;

Elf_Scn *getSectionByName(Elf* elf, const char *name);

#define E_ASSERT(call) if(!(call)) { \
                errx(EXIT_FAILURE, "libelf call failed in %s:%u: %s.", __FILE__, __LINE__, elf_errmsg(-1)); \
            }


#endif //ELFPATCHER_ELFPATCHER_H
