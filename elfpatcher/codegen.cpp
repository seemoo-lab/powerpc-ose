
#include <err.h>
#include <fcntl.h>
#include <gelf.h>
#include <string>
#include <iostream>
#include <map>
#include <libelf.h>
#include <elf.h>
#include <cstring>
#include <fstream>
#include <algorithm>
#include "asm.h"
#include "elfpatcher.h"
#include "codegen.h"
#include "reader.h"
#include "utils.h"
#include "virtual.h"

struct asmbuf {
    uint8_t buf[6*1024*1024]; // 6mb, just to have space
    uint32_t off;
    uint32_t base;
};
struct asmbuf codebuf;

uint32_t push(uint32_t instruction) {
    sbr(codebuf.buf + codebuf.off, instruction);
    codebuf.off+=4;
    return codebuf.base + codebuf.off - 4;
}
// space for registers...and margin to be safe
// the stack layout is shared with the access functions in include/orig.h
// word offsets:
// 0-1: reserved for callee
// 2: return address
// 3: pointer to original symbol name
// 4-14: registers R2-R12
// 15: register 14
// 16: original(or call stub) function address
const int private_stack_memory = 72;
uint32_t asm_saving_function;
uint32_t asm_restoring_function;

uint32_t asm_define_saving_function() {
    auto entry = push(PPC_STW(R0, SP, 2*4)); // store R0 on stack

    // save volatile registers 2-12
    // https://www.ibm.com/support/knowledgecenter/en/ssw_aix_72/com.ibm.aix.alangref/idalangref_reg_use_conv.htm
    for(uint32_t i=2; i<=12; i++) {
        // store register i+2 to 32bit stack offset i
        push(PPC_STW(i, SP, (i+2)*4));
    }
    // save R14
    push(PPC_STW(R14, SP, 15*4));
    // copy our SP in R14. this register is reserved in our c hooks and allows them to access our saved registers
    push(PPC_MR(R14, SP));
    push(PPC_BLR);
    return entry;
}

uint32_t asm_define_restoring_function() {

    // use 3 as first one so that we can just skip 3 and 4 by adding offset 8 to the address
    auto entry = push(PPC_LWZ(3, SP, (3+2)*4));
    for(uint32_t i=4; i<=12; i++) {
        push(PPC_LWZ(i, SP, (i+2)*4));
    }
    push(PPC_LWZ(2, SP, (2+2)*4));
    push(PPC_BLR);
    return entry;
}


uint32_t asm_save_context(uint32_t origAddr, const std::string &hooksym) {

    // write symbol name to memory
    const char *str = hooksym.c_str();
    strcpy((char*)codebuf.buf + codebuf.off, str);
    uint32_t symstrptr = codebuf.base + codebuf.off;
    codebuf.off += hooksym.length()+1;
    if(codebuf.off%4 != 0)
        codebuf.off += 4-codebuf.off%4; // 32bit align

    // asm entry point
    uint32_t addr = push(PPC_STWU(SP, SP, -private_stack_memory)); // lower stack pointer
    push(PPC_MFLR_R0); // move link register to R0
    // call the register saving function
    push(PPC_BLA(asm_saving_function));

    // save the original access for out hooks
    push(PPC_LIS(R9, origAddr>>16)); // 16 upper bits in R9
    push(PPC_ORI(R9, R9, origAddr)); // 16 upper bits ORed in R9
    push(PPC_STW(R9, SP, 16*4));
    // restore R9

    // save pointer to hooksym
    push(PPC_LIS(R9, symstrptr>>16)); // 16 upper bits in R9
    push(PPC_ORI(R9, R9, symstrptr)); // 16 upper bits ORed in R9
    push(PPC_STW(R9, SP, 3*4));

    push(PPC_LWZ(R9, SP, (2+9)*4));
    return addr;
}

void asm_restore_stack() {
    push(PPC_LWZ(R0, SP, 2*4)); // restore link register to R0
    push(PPC_MTLR_R0); // move link register from R0
    push(PPC_ADDI(SP, SP, private_stack_memory)); // restore stack pointer
}

void asm_restore_registers(bool ignore_return_registers) {
    if(ignore_return_registers) {
        push(PPC_BLA(asm_restoring_function + 8));  // R3 and R3 are for return values. jump over those
    } else {
        push(PPC_BLA(asm_restoring_function));
    }
}

void asm_restore_r14() {
    push(PPC_LWZ(R14, SP, 16*4));
}

void asm_jump(uint32_t address) {
    // TODO use count register to support long jumps
    push(PPC_BA(address));
}

void asm_call(uint32_t address) {
    // TODO use count register to support long jumps
    push(PPC_BLA(address));
}

uint32_t asm_precall(uint32_t hookAddr, uint32_t origAddr, const std::string &hooksym) {
    auto addr = asm_save_context(origAddr, hooksym);
    // call the hook
    asm_call(hookAddr);
    // restore context for original func
    asm_restore_registers(false);
    asm_restore_stack();
    asm_restore_r14();
    // jump to destination
    asm_jump(origAddr);
    return addr;
}

uint32_t asm_postcall(uint32_t hookAddr, uint32_t origAddr, const std::string &hooksym) {
    auto addr = asm_save_context(origAddr, hooksym);
    // call the destination
    asm_call(origAddr);
    // restore context for hook
    asm_restore_registers(false);
    // call the hook
    asm_call(hookAddr);
    asm_restore_r14();
    asm_restore_stack();
    // return
    push(PPC_BLR);
    return addr;
}

uint32_t asm_replace(uint32_t hookAddr, uint32_t origAddr, const std::string &hooksym) {
    auto addr = asm_save_context(origAddr, hooksym);
    // jump to hook
    asm_call(hookAddr);
    asm_restore_registers(true); // preserve return value
    asm_restore_r14();
    asm_restore_stack();
    // return
    push(PPC_BLR);
    return addr;
}

/*
void hookSymbol(const std::string &symbol, uint32_t callAddr) {
    uint32_t symaddr = symbols[symbol];
    hooks[symaddr] = codebuf.base + codebuf.off;

    addCallCode(callAddr, symbols[symbol]);
}
*/

std::map<std::string, uint32_t> copyCodeFromCompiledElf(const char *compiled_elf) {

    // read compiled elf and copy all from .compiled into our address space

    int fd = open(compiled_elf, O_RDWR, 0);
    if (fd < 0)
        err(EXIT_FAILURE, " open of linked.elf failed ");

    Elf *src = elf_begin(fd, ELF_C_RDWR, nullptr);
    E_ASSERT(dstelf!=nullptr);
    E_ASSERT(elf_kind(dstelf) == ELF_K_ELF);
    std::map<std::string, uint32_t> src_symtab;
    readSymbolTable(src, src_symtab);
    Elf_Scn *scn = getSectionByName(src, ".compiled");

    Elf_Data *data = nullptr;
    GElf_Shdr shdr; // NOLINT
    E_ASSERT(gelf_getshdr(scn, &shdr) != nullptr);
    size_t n = 0;
    while (n < shdr.sh_size && (data = elf_getdata(scn, data)) != nullptr) {
        memcpy(codebuf.buf + codebuf.off, data->d_buf, data->d_size);
        codebuf.off += data->d_size;
    }
    codebuf.off += 4-codebuf.off%4; // 32bit align
    return src_symtab;
}

bool firstInstructionIsMovable(uint32_t addr) {
    // check if the first instruction in a symbol can be moved into a call stub and replaced by a branch into the trampoline
    uint32_t instruction = readVirtualAddress(dstelf, addr);

    // the compiler generated code uses this two instructions at first nearly every time if it the function uses a stack
    // and is not a leaf function
    return (instruction & PPC_MTLR_R0 || instruction & PPC_STW_MASK);
}

uint32_t generateCallStub(uint32_t addr) {
    // generate a call stub to reach the original function flow.
    // write the first original instruction (that gets to be replaced with a branch) and then jump to the next
    // instruction in the original code
    uint32_t instruction = readVirtualAddress(dstelf, addr);
    uint32_t stub_addr = push(instruction);
    push(PPC_BA(addr+4));
    return stub_addr;
}

// TODO extend with wildcard in front of function name
void generateCode(const char *patchconfig, const char *compiled_elf) {
    // *patchconfig = hooks.txt
    // *compiled_elf = hooks.elf

    auto dstsyms = copyCodeFromCompiledElf(compiled_elf);

    asm_saving_function = asm_define_saving_function();
    asm_restoring_function = asm_define_restoring_function();

    // read the patch config. each line is:
    // <PRE|POST|REPLACE> <symbol|0x address> <dstsymbol>
    //  ^                  ^                ^- destination symbol in the compiled elf
    //  |                   `- symbol (name) or address (0x prefixed) in the original elf
    //   `- what to do:
    //       - PRE: prehook, call dstsymbol and then the original symbol
    //       - POST: posthook, call the original symbol and then dstsymbol
    //       - REPLACE: only call dstsymbol
    std::ifstream input(patchconfig);
    printf("Hooks: \n");
    for( std::string line; getline( input, line ); )
    {
        trim(line);
        // TODO One could also remove inline comments after an actual statement
        if(line.size() == 0 || line[0] == '#') continue;
        auto e = split(line, ' ');
        if(e.size() != 3) {
            errx(EXIT_FAILURE, "Invalid config line: %s\n", line.c_str());
        }

        // type,hookstr,dstsym = <PRE|POST|REPLACE>, <symbol|0x address>, <dstsymbol>
        std::string type = e[0];
        if(type == "PRE" || type == "POST" || type == "REPLACE") {
            std::string hookstr = e[1];
            std::string dstsym = e[2];

            std::cout << hookstr << std::endl;

            for (auto const &pair : symbols) {
                std::string hooksym = pair.first;

                if (pair.second == 0) continue;

                if (!match(hookstr.c_str(), hooksym.c_str(), 0, 0)) continue;


                std::cout << " - found matching hooksym" << std::endl;

                uint32_t hookaddr; // address of the compiled hook
                uint32_t symaddr; // address of the original function

                if (hooksym[0] == '0' && hooksym[1] == 'x') { // string starts with 0x
                    symaddr = static_cast<uint32_t>(atoi(hooksym.c_str() + 2));
                } else {
                    if (symbols.find(hooksym) == symbols.end()) {
                        errx(EXIT_FAILURE, "source symbol not found: %s\n", hooksym.c_str());
                    }
                    symaddr = symbols[hooksym];
                }

                if (hooks.find(symaddr) != hooks.end()) {
                    warn("trying to double-hook symbol %s. using first one.\n", dstsym.c_str());
                    continue;
                }

                if (dstsyms.find(dstsym) == dstsyms.end()) {
                    errx(EXIT_FAILURE, "dest symbol not found: %s\n", dstsym.c_str());
                }
                hookaddr = dstsyms[dstsym];

                if (symaddr > 0x275254) continue; // known max code address. this is a data symbol


                auto origCallAddr = symaddr;
                bool useStub = false;

                if (firstInstructionIsMovable(symaddr)) {
                    // use a callstub
                    useStub = true;
                    origCallAddr = generateCallStub(symaddr);
                }


                if (type == "PRE") {
                    hooks[origCallAddr] = asm_precall(hookaddr, origCallAddr, hooksym);
                } else if (type == "POST") {
                    hooks[origCallAddr] = asm_postcall(hookaddr, origCallAddr, hooksym);
                } else if (type == "REPLACE") {
                    hooks[origCallAddr] = asm_replace(hookaddr, origCallAddr, hooksym);
                }

                if (useStub) {
                    writeVirtualAddress(dstelf, symaddr, PPC_BA(hooks[origCallAddr]));
                } else {
                    printf(" - IGNORING, no mtlr or stw: %s\n", hooksym.c_str());
                }

                printf(" - %s %s (%s, 0x%x) -> %s (0x%x)\n", type.c_str(), hooksym.c_str(),
                       useStub ? "callstub" : "branch patching",
                       symaddr, dstsym.c_str(), hookaddr);
            }

            std::cout << " - done searching for matching hooksymbols" << std::endl;
        }
        else if(type == "BYTE" || type == "SHORT" || type == "LONG") {

            std::string addrstr = e[1];
            std::string valuestr = e[2];
            uint32_t addr, value;
            if (addrstr[0] == '0' && addrstr[1] == 'x') { // string starts with 0x
                addr = static_cast<uint32_t>(atoi(addrstr.c_str() + 2));
            } else {
                fprintf(stderr, "Invalid config address must start with 0x\n");
                exit(EXIT_FAILURE);
            }
            if (valuestr[0] == '0' && valuestr[1] == 'x') { // string starts with 0x
                value = static_cast<uint32_t>(atoi(valuestr.c_str() + 2));
            } else {
                fprintf(stderr, "Invalid config value must start with 0x\n");
                exit(EXIT_FAILURE);
            }
            uint32_t oldVal = readVirtualAddress(dstelf, addr);
            if(type == "BYTE") {
                value = (oldVal & 0xffffff00) | (value & 0x000000ff);
            }
            if(type == "SHORT") {
                value = (oldVal & 0xffff0000) | (value & 0x0000ffff);
            }
            writeVirtualAddress(dstelf, addr, value);
        }
        else {
            fprintf(stderr, "Invalid config key: %s\n", e[0].c_str());
            exit(EXIT_FAILURE);
        }
    }

    //printf("hook(): 0x%x\n", sym["hook"]);
    //hookSymbol("HdwInit", sym["hook"]);
    //hookSymbol("printf", sym["hook"]);

}


void createCodeSegment(Elf_Scn *tpl_scn, const char *patchconfig, const char *compiled_elf) {
    Elf32_Shdr *tpl_shdr = elf32_getshdr(tpl_scn);
    Elf_Data *tpl_data = elf_getdata(tpl_scn, nullptr);

    Elf_Data *data;
    Elf_Scn *scn = nullptr;

    E_ASSERT((scn = elf_newscn(dstelf)) != nullptr);
    E_ASSERT((data = elf_newdata(scn)) != nullptr);

    data->d_align = tpl_data->d_align;
    data->d_type = tpl_data->d_type;
    data->d_version = tpl_data->d_version;
    data->d_off = 0;

    size_t numPHeaders;
    elf_getphdrnum(dstelf, &numPHeaders);

    Elf_Scn *names = getSectionByName(dstelf, ".shstrtab");

    Elf_Data *ndata = nullptr;
    ndata = elf_getdata(names, ndata);
    //hexDump(ndata->d_buf, ndata->d_size);
    auto *x = (char*)ndata->d_buf;
    x[ndata->d_size-3] = '.';
    x[ndata->d_size-2] = 'x';


    Elf32_Shdr *shdr = elf32_getshdr(scn);

    shdr->sh_name = ndata->d_size-3;
    shdr->sh_type = tpl_shdr->sh_type;
    shdr->sh_flags = tpl_shdr->sh_flags;
    shdr->sh_entsize = 0;


    // base address of original firmware is 0x40000. we just define that our addresses start at 0x02500 which
    // give us 246kb of memory.
    // THIS MUST BE IN SYNC WITH link.x!!!
    codebuf.base = 0x02500;
    uint32_t max_size = 0x40000 - codebuf.base;
    generateCode(patchconfig, compiled_elf);

    data->d_buf = codebuf.buf;
    data->d_size = codebuf.off;
    shdr->sh_addr = codebuf.base;
    printf("size: %u/%u KiB (%u%%)\n", codebuf.off / 1024, max_size / 1024, 100 * codebuf.off / max_size);
    if(codebuf.base + codebuf.off > 0x40000) {
        fprintf(stderr, "ERROR: overwriting original firmware virtual memory.\n");
        exit(EXIT_FAILURE);
    }


#if 0

    Elf_Data *romdata;
    Elf_Scn *romscn = nullptr;
    E_ASSERT((romscn = elf_newscn(dstelf)) != nullptr);
    E_ASSERT((romdata = elf_newdata(romscn)) != nullptr);

    romdata->d_align = tpl_data->d_align;
    romdata->d_type = tpl_data->d_type;
    romdata->d_version = tpl_data->d_version;
    romdata->d_off = 0;


    Elf32_Shdr *romshdr = elf32_getshdr(romscn);

    romshdr->sh_name = 0x141;
    romshdr->sh_type = tpl_shdr->sh_type;
    romshdr->sh_flags = tpl_shdr->sh_flags;
    romshdr->sh_entsize = 0;

    FILE *f = fopen("/home/uwe/0xff000000.bin", "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  //same as rewind(f);

    char *rom = (char*)malloc(fsize);
    fread(rom, fsize, 1, f);
    fclose(f);

    romdata->d_buf = rom;
    romdata->d_size = fsize;
    romshdr->sh_addr = 0xff000000;
#endif


    E_ASSERT(elf_update(dstelf, ELF_C_WRITE) >= 0);


    size_t n;
    E_ASSERT(elf_getphdrnum(dstelf, &n) == 0)


    // we need to insert a new program header into the elf to tell the bootloader to load our section into its ram.
    // the program header is at the very beginning of the file and every program header has a pointer into the file
    // where is data is stored (p_offset)
    // sadly, we cannot just add a new entry but need to create a whole new program header. libelf then looses track
    // of this entries and as a result, our offsets are all wrong.
    // I don't see a direct reference from a program header entry to its section, so to get the new offsets
    // i just read the offset of our new section, create a new program header with n+1 entries, read the offset
    // again and add this diff to every other offset.

    // copy the existing header into a local buffer

    size_t newHdrs = numPHeaders+1;

    GElf_Phdr *gphdr = new GElf_Phdr[newHdrs];
    GElf_Phdr gptr;
    GElf_Phdr *phdr = &gphdr[numPHeaders]; // our new entry


    phdr->p_type = PT_LOAD;
    phdr->p_align = 1;
    phdr->p_flags = PF_R | PF_X;
    phdr->p_vaddr = shdr->sh_addr;
    phdr->p_paddr = shdr->sh_addr;
    phdr->p_filesz = data->d_size;
    phdr->p_memsz = data->d_size;
    phdr->p_offset = shdr->sh_offset;

    // read the existing ones.
    for(int i=0;i<numPHeaders;i++) {
        E_ASSERT (gelf_getphdr(dstelf, i, &gptr) == &gptr);
        memcpy(&gphdr[i], &gptr, sizeof(GElf_Phdr));
    }

#if 0
    newHdrs = 9;
    E_ASSERT ((elf32_newphdr(dstelf, 9)) != NULL);
    gphdr[8].p_type = PT_LOAD;
    gphdr[8].p_align = 1;
    gphdr[8].p_flags = PF_R | PF_X;
    gphdr[8].p_vaddr = romshdr->sh_addr;
    gphdr[8].p_paddr = romshdr->sh_addr;
    gphdr[8].p_filesz = romdata->d_size;
    gphdr[8].p_memsz = romdata->d_size;
    gphdr[8].p_offset = romshdr->sh_offset;
#else
    // new, empty header
    E_ASSERT ((elf32_newphdr(dstelf, numPHeaders+1)) != NULL);
#endif

    // this is required that shdr->sh_offset gets the new offset
    E_ASSERT(elf_update(dstelf, ELF_C_WRITE) >= 0);

    // phdr->p_offset has our old offset
    Elf64_Off offsetDiff = shdr->sh_offset - phdr->p_offset;

    for(int i=0;i<newHdrs;i++) {
        // add our offset diff to every entry
        gphdr[i].p_offset += offsetDiff;
        gphdr[i].p_paddr = gphdr[i].p_vaddr;
        gelf_update_phdr(dstelf, i, &gphdr[i]);
    }
    for(int i=0;i<newHdrs;i++) {
        E_ASSERT (gelf_getphdr(dstelf, i, &gptr) == &gptr);
    }

    //hexDump(ndata->d_buf, ndata->d_size);

    E_ASSERT(elf_update(dstelf, ELF_C_WRITE) >= 0);

    delete[] gphdr;
}
