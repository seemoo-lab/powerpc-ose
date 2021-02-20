
#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <map>
#include <elf.h>
#include <cstring>
#include "utils.h"
#include "asm.h"
#include "elfpatcher.h"
#include "codegen.h"
#include "reader.h"

void patchBranchInstructions(Elf_Scn *scn);

Elf *dstelf;
std::map<uint32_t, uint32_t> hooks;
std::map<std::string, uint32_t> symbols;


int main(int argc, char **argv) {
    if (argc != 5)
        errx(EXIT_FAILURE, " usage : %s <input elf> <compiled elf> <patch config> <output elf>", argv[0]);
    E_ASSERT(elf_version(EV_CURRENT) != EV_NONE);
    // copy input file and then work on the copied version
    if (copyFile(argv[1], argv[4]) != 0)
        err(EXIT_FAILURE, R"( File copy from "%s" to "%s" failed)", argv[1], argv[4]);
    
    int fd = open(argv[4], O_RDWR, 0);
    if (fd < 0)
        err(EXIT_FAILURE, " open \"%s\" failed ", argv[2]);

    dstelf = elf_begin(fd, ELF_C_RDWR, nullptr);
    E_ASSERT(dstelf!=nullptr);
    E_ASSERT(elf_kind(dstelf) == ELF_K_ELF);



    // find symbol table and read it
    readSymbolTable(dstelf, symbols);

    createCodeSegment(getSectionByName(dstelf, ".text"), argv[3], argv[2]);

    // somehow most of the sections are SHT_PROGBITS aka "program data". just use the section name to know
    // which one to patch
    //patchBranchInstructions(getSectionByName(dstelf, ".corebootstrap"));
    //patchBranchInstructions(getSectionByName(dstelf, ".text"));


    E_ASSERT(elf_update(dstelf, ELF_C_WRITE) >= 0);

    elf_end(dstelf);
    close(fd);
    exit(EXIT_SUCCESS);
}


void patchBranchInstructions(Elf_Scn *scn) {
    Elf_Data *data = nullptr;
    GElf_Shdr shdr; // NOLINT
    E_ASSERT(gelf_getshdr(scn, &shdr) != nullptr);
    size_t n = 0;
    elf_flagscn(scn, ELF_C_SET, ELF_F_DIRTY);
    while (n < shdr.sh_size && (data = elf_getdata(scn, data)) != nullptr) {
        n += data->d_size;
//        hexDump(data->d_buf, data->d_size);

        // luckily we have no variable instruction length. every instruction is 32bit wide
        auto *d = (uint8_t *)data->d_buf;
        for(int i=0; i<data->d_size; i+=4) {
            // reverse byte order
            uint32_t instr = ((uint32_t )d[i+0])<<24 |
                             ((uint32_t )d[i+1])<<16 |
                             ((uint32_t )d[i+2])<<8 |
                             ((uint32_t )d[i+3]);
            uint32_t newInstr = instr;
            // we assume that we are running on a LSB machine and patching a MSB binary.
            // the branch instruction has the first 6 bits as 18 decimal.
            // b:     18d = 010010b (lsb) = 010010b(msb)
            // bc:    16d = 010000b (lsb) = 000010b(msb)
            // bcctr: 19d = 010011b (lsb) = 010011b(msb)
            // TODO other branch instructions like bc
            // TODO dont forget .data...
            // https://www.ibm.com/support/knowledgecenter/en/ssw_aix_71/com.ibm.aix.alangref/idalangref_bbranchinst.htm
            //uint32_t bits_branch_cond = 0b00001000000000000000000000000000;
            uint32_t instr_6b = instr & INSTR_MASK;
            auto addr = (uint32_t)shdr.sh_addr + i;
            if(instr_6b == BITS_BRANCH) {


                // calculate destination address of branch
                // naming as in the ibm knowledge base
                int32_t LL = instr & B_LL_MASK;
                uint32_t AA = instr & B_AA;
                uint32_t LK = instr & B_LK;

                // we have no absolute jumps in our dstelf. skip this because it is a string or constant
                if(AA) continue;

                // sign-extend LL
                if(LL & 0b0000001000000000000000000000000) {
                    // 24bit sign bit is set -> extend bits to 32bit
                    LL |= 0b11111100000000000000000000000000;
                }
                uint32_t dest = addr + LL;
                if(hooks.find(dest) != hooks.end()) {
                    uint32_t hookaddr = hooks[dest];
                    int32_t newLL = hookaddr - addr;
                    newInstr = PPC_BRANCH(newLL, false, LK);

                    printf("hook branch at 0x%x: 0x%x to 0x%x (instruction 0x%x to 0x%x)\n",
                           addr, dest, hookaddr, instr, newInstr);
                    if (mayBeString(instr)) {
                        printf("WARNING: maybe a string at 0x%x, branch destination 0x%x", addr, dest);
                    }
                }
            }

            if(instr != newInstr) {
                // update instruction in buffer
                sbr(d + i, newInstr);
            }
        }

    }
}
