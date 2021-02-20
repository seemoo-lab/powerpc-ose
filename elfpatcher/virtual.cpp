
#include "virtual.h"
#include <err.h>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <map>
#include <elf.h>
#include <libelf.h>
#include "utils.h"
#include "elfpatcher.h"



uint32_t readWriteVirtualAddress(Elf *elf, uint32_t addr, bool write, uint32_t val) {

    Elf_Scn *scn = nullptr;
    size_t shstrtab; // section header string table
    E_ASSERT(elf_getshdrstrndx(elf, &shstrtab) == 0)
    while ((scn = elf_nextscn(elf, scn)) != nullptr) {
        GElf_Shdr shdr; // NOLINT
        E_ASSERT (gelf_getshdr(scn, &shdr) == &shdr)
        if(shdr.sh_addr < addr && shdr.sh_addr + shdr.sh_size > addr) {
            auto offset = addr - shdr.sh_addr;
            if(write) {
                elf_flagscn(scn, ELF_C_SET, ELF_F_DIRTY);
            }
            size_t n = 0;
            Elf_Data *data = nullptr;
            while (n < shdr.sh_size && (data = elf_getdata(scn, data)) != nullptr) {
                if(n < offset && n + data->d_size > offset && data->d_buf != NULL) {
                    // chunk of data matches our offset

                    auto data_offset = offset - n;
                    auto *d = (uint8_t *)data->d_buf;
                    if(write) {
                        sbr(d + data_offset, val);
                    }
                    uint32_t ret;
                    sbr((uint8_t*)&ret, *((uint32_t*)(d + data_offset)));
                    return ret;
                }
                n += data->d_size;
            }
        }

    }
    errx(EXIT_FAILURE, "readVirtualAddress() failed: address 0x%x not found. ", addr);
}

uint32_t readVirtualAddress(Elf *elf, uint32_t addr) {
    return readWriteVirtualAddress(elf, addr, false, 0);
}

void writeVirtualAddress(Elf *elf, uint32_t addr, uint32_t val) {
    readWriteVirtualAddress(elf, addr, true, val);
}
