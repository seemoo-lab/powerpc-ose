#!/bin/bash

# This script searches for BOOTFUNCs and BOOTTHREADs within the code and 
# generates a method to call them while booting OSE

CALLS=""

echo "#include <symbols.h>"

for file in "$@"; do
    for fn in $(grep -oP  "BOOTFUNC\s*\K(\S+)(?=\(\))" $file); do
        echo "void $fn();"
        CALLS="$CALLS\n    $fn();"
     done
    for fn in $(grep -oP  "BOOTTHREAD\s*\K(\S+)(?=\(\))" $file); do
        echo "void $fn();"
        CALLS="$CALLS\n    create_thread(OS_BG_PROC, \"$fn\", $fn);"
     done
done
echo -n "void gen_init() {"
echo -e $CALLS
echo "}"
