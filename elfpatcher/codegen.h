
#ifndef ELFPATCHER_CODEGEN_H
#define ELFPATCHER_CODEGEN_H

void createCodeSegment(Elf_Scn *tpl_scn, const char *patchconfig, const char *compiled_elf);

#endif //ELFPATCHER_CODEGEN_H
