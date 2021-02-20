
/*
 * Declarations necessary to call the original function ( orig_call() )from a hook function
 */


#ifndef ORIG_H
#define ORIG_H

#include <types.h>

#define TRAMPOLINE_REGISTER "14"


struct trampoline_stack_t {
    uint32_t _RESERVED_0;
    uint32_t _RESERVED_1;
    uint32_t LR;
    char *symbol_name;
    uint32_t R2;
    union {
        uint32_t R3;
        uint32_t arg_1;
        uint32_t ret;
        uint32_t ret_1;
    };
    union {
        uint32_t R4;
        uint32_t arg_2;
        uint32_t ret_2;
    };
    union {
        uint32_t R5;
        uint32_t arg_3;
    };
    union {
        uint32_t R6;
        uint32_t arg_4;
    };
    union {
        uint32_t R7;
        uint32_t arg_5;
    };
    union {
        uint32_t R8;
        uint32_t arg_6;
    };
    union {
        uint32_t R9;
        uint32_t arg_7;
    };
    union {
        uint32_t R10;
        uint32_t arg_8;
    };
    uint32_t R11;
    uint32_t R12;
    uint32_t R14;
    uint32_t symbol_address;
};

// the trampoline code saves its stack pointer in r14.
register struct trampoline_stack_t *trampoline_stack asm ("r" TRAMPOLINE_REGISTER);

static inline uint32_t orig_call() {
    uint32_t ret;
    __asm__ __volatile__(
    // save local context
        "mflr 0\n"
        "stwu 1, -64(1)\n"
        "stw 0, 8(1)\n"
        "stw 2, 12(1)\n"
        "stw 3, 16(1)\n"
        "stw 4, 20(1)\n"
        "stw 5, 24(1)\n"
        "stw 6, 28(1)\n"
        "stw 7, 32(1)\n"
        "stw 8, 36(1)\n"
        "stw 9, 40(1)\n"
        "stw 10, 44(1)\n"
        "stw 11, 48(1)\n"
        "stw 12, 52(1)\n"
    // load function call address into count register
        "lwz 9, 64(" TRAMPOLINE_REGISTER ")\n"
        "mtctr 9\n"

    // restore trampoline context
        "lwz 2, 16(" TRAMPOLINE_REGISTER ")\n"
        "lwz 3, 20(" TRAMPOLINE_REGISTER ")\n"
        "lwz 4, 24(" TRAMPOLINE_REGISTER ")\n"
        "lwz 5, 28(" TRAMPOLINE_REGISTER ")\n"
        "lwz 6, 32(" TRAMPOLINE_REGISTER ")\n"
        "lwz 7, 36(" TRAMPOLINE_REGISTER ")\n"
        "lwz 8, 40(" TRAMPOLINE_REGISTER ")\n"
        "lwz 9, 44(" TRAMPOLINE_REGISTER ")\n"
        "lwz 10, 48(" TRAMPOLINE_REGISTER ")\n"
        "lwz 11, 52(" TRAMPOLINE_REGISTER ")\n"
        "lwz 12, 56(" TRAMPOLINE_REGISTER ")\n"

    // call pointer in count register
        "bctrl\n"
        
    // save return value
        "mr %[ret], 3\n"

    // restore local context
        "lwz 0, 8(1)\n"
        /*
        "lwz 2, 12(1)\n"
        "lwz 3, 16(1)\n"
        "lwz 4, 20(1)\n"
        "lwz 5, 24(1)\n"
        "lwz 6, 28(1)\n"
        "lwz 7, 32(1)\n"
        "lwz 8, 36(1)\n"
        "lwz 9, 40(1)\n"
        "lwz 10, 44(1)\n"
        "lwz 11, 48(1)\n"
        "lwz 12, 52(1)\n"
         */
        "mtlr 0\n"
        "addi 1, 1, 64\n"
        :
        [ret] "=r"(ret)
    );

    return ret;
}


#endif
