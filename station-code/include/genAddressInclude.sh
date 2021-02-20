#!/bin/bash

# This script takes exactly one parameter which is a elf binary file. It then reads out the symbols defined in this binary and converts them to a c header file called "address.h". Whithin address.h, every symbol in the binary will have a "#define" entry mapping the symbol name to the corresponding hex address

set -e

if [ $# != 1 ] || [ ! -f $1 ]; then
    echo "usage: $0 <elf file>"
    exit
fi

echo "// AUTO GENERATED FILE BY $0" > address.h
echo "#ifndef SYM_ADDRESS_H" >> address.h
echo "#define SYM_ADDRESS_H" >> address.h

# The next line actually reads out the binary and creates the constants. It does the following:
# - remove every line which contains a dot
# - remove the line with the table headings 
# - remove every line which contains a slash
# - remove every empty line
# - then the awk call:
#   - the column $8 contains the name of the symbol
#   - ignore every line where the symbol name is empty
#   - !a[$8]++ ensures, that every line is only written once
#   - generate the c syntax with "#define ..." with the symbol name and the corresponding hex address
readelf -s --wide $1 | grep -v "\." | grep -v "Num:[[:space:]]*Value" | grep -v "/" | sed '/^$/d' | \
        awk '{if($8 != "" && !a[$8]++) print "#define SYM_ADDR_"$8" 0x"$2 }' >> address.h
echo "#endif" >> address.h
