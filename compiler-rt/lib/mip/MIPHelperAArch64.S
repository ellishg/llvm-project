#if defined(__aarch64__) || defined(__arm64__)
#include "../sanitizer_common/sanitizer_asm.h"

// The address of the raw profile symbol is expected to be in the x16 register.
#define PROFILE_REGISTER x16

    .text
    .p2align 2

.macro SAVE_STACK_FRAME
    str     x8, [sp, #-0xE0]!
    stp     x7, x6, [sp, #0x10]
    stp     x5, x4, [sp, #0x20]
    stp     x3, x2, [sp, #0x30]
    stp     x1, x0, [sp, #0x40]
    stp     q7, q6, [sp, #0x50]
    stp     q5, q4, [sp, #0x70]
    stp     q3, q2, [sp, #0x90]
    stp     q1, q0, [sp, #0xB0]
    stp     x29, x30, [sp, #0xD0]
    add     x29, sp, #0xD0
    CFI_DEF_CFA(w29, 0x10)
    CFI_OFFSET(w30, -0x08)
    CFI_OFFSET(w29, -0x10)
.endm

.macro RESTORE_STACK_FRAME
    ldp     x29, x30, [sp, #0xD0]
    ldp     q1, q0, [sp, #0xB0]
    ldp     q3, q2, [sp, #0x90]
    ldp     q5, q4, [sp, #0x70]
    ldp     q7, q6, [sp, #0x50]
    ldp     x1, x0, [sp, #0x40]
    ldp     x3, x2, [sp, #0x30]
    ldp     x5, x4, [sp, #0x20]
    ldp     x7, x6, [sp, #0x10]
    ldr     x8, [sp], #0xE0
.endm

// Call the C instrumentation helper for function call counts. Expected to be
// called directly from the instrumented function.
    .global ASM_SYMBOL(__llvm_mip_call_counts_caller)
    ASM_TYPE_FUNCTION(ASM_SYMBOL(__llvm_mip_call_counts_caller))
ASM_SYMBOL(__llvm_mip_call_counts_caller):
    CFI_STARTPROC
    SAVE_STACK_FRAME
    mov     x0, PROFILE_REGISTER
    // void *Helper(uint64_t ProfileDataAddress)
    bl      ASM_SYMBOL(__llvm_mip_call_counts_instrumentation_helper)

    RESTORE_STACK_FRAME
    // Restore x29, x30 and return to the caller as if we were never called.
    mov     x16, x30
    ldp     x29, x30, [sp], #0x10
    ret     x16
    CFI_ENDPROC
    ASM_SIZE(__llvm_mip_call_counts_caller)

#endif // defined(__aarch64__) || defined(__arm64__)
