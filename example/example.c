
#include <symbols.h>
#include <types.h>

void pre_printf(const char *format) {
    uint32_t regs[32];
    __asm__ __volatile__(
        "stw 0, %[r0] ;"
        "stw 1, %[r1] ;"
        "stw 2, %[r2] ;"
        "stw 3, %[r3] ;"
        "stw 4, %[r4] ;"
        "stw 5, %[r5] ;"
        "stw 6, %[r6] ;"
        "stw 7, %[r7] ;"
        "stw 8, %[r8] ;"
        "stw 9, %[r9] ;"
        "stw 10, %[r10] ;"
        "stw 11, %[r11] ;"
        "stw 12, %[r12] ;"
        "stw 13, %[r13] ;"
        "stw 14, %[r14] ;"
        "stw 15, %[r15] ;"
        :
        [r0] "=m" (regs[0]),
        [r1] "=m" (regs[1]),
        [r2] "=m" (regs[2]),
        [r3] "=m" (regs[3]),
        [r4] "=m" (regs[4]),
        [r5] "=m" (regs[5]),
        [r6] "=m" (regs[6]),
        [r7] "=m" (regs[7]),
        [r8] "=m" (regs[8]),
        [r9] "=m" (regs[9]),
        [r10] "=m" (regs[10]),
        [r11] "=m" (regs[11]),
        [r12] "=m" (regs[12]),
        [r13] "=m" (regs[13]),
        [r14] "=m" (regs[14]),
        [r15] "=m" (regs[15])
    );
    __asm__ __volatile__(
        "stw 16, %[r16] ;"
        "stw 17, %[r17] ;"
        "stw 18, %[r18] ;"
        "stw 19, %[r19] ;"
        "stw 20, %[r20] ;"
        "stw 21, %[r21] ;"
        "stw 22, %[r22] ;"
        "stw 23, %[r23] ;"
        "stw 24, %[r24] ;"
        "stw 25, %[r25] ;"
        "stw 26, %[r26] ;"
        "stw 27, %[r27] ;"
        "stw 28, %[r28] ;"
        "stw 29, %[r29] ;"
        "stw 30, %[r30] ;"
        "stw 31, %[r31] ;"
        :
        [r16] "=m" (regs[16]),
        [r17] "=m" (regs[17]),
        [r18] "=m" (regs[18]),
        [r19] "=m" (regs[19]),
        [r20] "=m" (regs[20]),
        [r21] "=m" (regs[21]),
        [r22] "=m" (regs[22]),
        [r23] "=m" (regs[23]),
        [r24] "=m" (regs[24]),
        [r25] "=m" (regs[25]),
        [r26] "=m" (regs[26]),
        [r27] "=m" (regs[27]),
        [r28] "=m" (regs[28]),
        [r29] "=m" (regs[29]),
        [r30] "=m" (regs[30]),
        [r31] "=m" (regs[31])
    );
    printf("pre_printf ");
    for(int i=0;i<32;i++) {
        printf("r%i: %x ", i, regs[i]);
    }
    printf("\n");
}

void post_printf() {
    printf("post_printf\n");
}
