# Linker script
The `link.x` linker script tells the linker to just put everything in one ELF section,
starting at virtual address `0x02500`.

It just does not matter if it is in multiple or in one section and so we only have to copy
one into our destination ELF.
