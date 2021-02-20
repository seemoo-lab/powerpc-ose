
#include "reader.h"
#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <map>
#include <elf.h>
#include <cstring>
#include <gelf.h>
#include "utils.h"
#include "elfpatcher.h"

size_t strtab; // string table

void readSymbolTable(Elf* elf, std::map<std::string, uint32_t> &table) {
    Elf_Scn *scn = getSectionByName(elf, ".symtab");
    Elf_Data *data = nullptr;
    GElf_Shdr shdr; // NOLINT
    E_ASSERT(gelf_getshdr(scn, &shdr) != nullptr);
    size_t n = 0;
    size_t strtab = elf_ndxscn(getSectionByName(elf, ".strtab"));
    while (n < shdr.sh_size && (data = elf_getdata(scn, data)) != nullptr) {
        n += data->d_size;
        size_t cnt = data->d_size / sizeof(Elf32_Sym);
        for(int i=0; i<cnt; i++) {
            GElf_Sym sym; // NOLINT
            E_ASSERT(gelf_getsym(data, i, &sym) == &sym);
            //hexDump(&sym, sizeof(sym));

            //printf("0x%x\n", sym.st_info);
            //if(sym.st_info == STT_FUNC) {
            //std::cout<<"symbol: "<<getName(strtab, sym.st_name)<<std::endl;
            auto name = getName(elf, strtab, sym.st_name);

            // function is in section number "sym.st_shndx" at address "sym.st_value"

            table[name] = (uint32_t )sym.st_value;

        }
    }
}

Elf_Scn *getSectionByName(Elf *elf, const char *name) {
    Elf_Scn *scn = nullptr;
    size_t shstrtab; // section header string table
    E_ASSERT(elf_getshdrstrndx(elf, &shstrtab) == 0)
    while ((scn = elf_nextscn(elf, scn)) != nullptr) {
        GElf_Shdr shdr; // NOLINT
        if (gelf_getshdr(scn, &shdr) != &shdr)
            errx(EXIT_FAILURE, " getshdr() failed: %s . ", elf_errmsg(-1));
        if(getName(elf, shstrtab, shdr.sh_name) == name) return scn;
    }
    errx(EXIT_FAILURE, "getSectionByName() failed: section %s not found. ", name);
}

std::string getName(Elf *elf, size_t table, size_t offset) {
    const char *c = elf_strptr(elf, table, offset);
    //E_ASSERT(c != nullptr);
    return std::string(c);
}
