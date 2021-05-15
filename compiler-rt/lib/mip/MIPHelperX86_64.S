#if defined(__x86_64__)
#include "../sanitizer_common/sanitizer_asm.h"

// The address of the raw profile symbol is expected to be in the rax register.
#define PROFILE_REGISTER %rax

    .text
    .p2align 2

.macro ALIGN_STACK_16B
#if defined(__APPLE__)
    subq    $$8, %rsp
#else
    subq    $8, %rsp
#endif
    CFI_ADJUST_CFA_OFFSET(8)
.endm

.macro RESTORE_STACK_ALIGNMENT
#if defined(__APPLE__)
    addq    $$8, %rsp
#else
    addq    $8, %rsp
#endif
    CFI_ADJUST_CFA_OFFSET(-8)
.endm

.macro SAVE_STACK_FRAME
    pushfq
    CFI_ADJUST_CFA_OFFSET(8)
#if defined(__APPLE__)
    subq    $$240, %rsp
#else
    subq    $240, %rsp
#endif
    CFI_ADJUST_CFA_OFFSET(240)
    movq    %rbp, 232(%rsp)
    movupd  %xmm0, 216(%rsp)
    movupd  %xmm1, 200(%rsp)
    movupd  %xmm2, 184(%rsp)
    movupd  %xmm3, 168(%rsp)
    movupd  %xmm4, 152(%rsp)
    movupd  %xmm5, 136(%rsp)
    movupd  %xmm6, 120(%rsp)
    movupd  %xmm7, 104(%rsp)
    movq    %rdi, 96(%rsp)
    movq    %rax, 88(%rsp)
    movq    %rdx, 80(%rsp)
    movq    %rsi, 72(%rsp)
    movq    %rcx, 64(%rsp)
    movq    %r8, 56(%rsp)
    movq    %r9, 48(%rsp)
    movq    %r10, 40(%rsp)
    movq    %r11, 32(%rsp)
    movq    %r12, 24(%rsp)
    movq    %r13, 16(%rsp)
    movq    %r14, 8(%rsp)
    movq    %r15, 0(%rsp)
.endm

.macro RESTORE_STACK_FRAME
    movq    232(%rsp), %rbp
    movupd  216(%rsp), %xmm0
    movupd  200(%rsp), %xmm1
    movupd  184(%rsp), %xmm2
    movupd  168(%rsp), %xmm3
    movupd  152(%rsp), %xmm4
    movupd  136(%rsp), %xmm5
    movupd  120(%rsp), %xmm6
    movupd  104(%rsp), %xmm7
    movq    96(%rsp), %rdi
    movq    88(%rsp), %rax
    movq    80(%rsp), %rdx
    movq    72(%rsp), %rsi
    movq    64(%rsp), %rcx
    movq    56(%rsp), %r8
    movq    48(%rsp), %r9
    movq    40(%rsp), %r10
    movq    32(%rsp), %r11
    movq    24(%rsp), %r12
    movq    16(%rsp), %r13
    movq    8(%rsp), %r14
    movq    0(%rsp), %r15
#if defined(__APPLE__)
    addq    $$240, %rsp
#else
    addq    $240, %rsp
#endif
    CFI_ADJUST_CFA_OFFSET(-240)
    popfq
    CFI_ADJUST_CFA_OFFSET(-8)
.endm

// Call the C instrumentation helper for function call counts. Expected to be
// called directly from the instrumented function.
    .global ASM_SYMBOL(__llvm_mip_call_counts_caller)
    ASM_TYPE_FUNCTION(ASM_SYMBOL(__llvm_mip_call_counts_caller))
ASM_SYMBOL(__llvm_mip_call_counts_caller):
    CFI_STARTPROC
    // NOTE: When __llvm_mip_call_counts_caller is the very first instruction
    //       of the instrumented function, the stack is already aligned.
    // ALIGN_STACK_16B
    SAVE_STACK_FRAME
    movq    PROFILE_REGISTER, %rdi
    // void *Helper(uint64_t ProfileDataAddress)
    callq   ASM_SYMBOL(__llvm_mip_call_counts_instrumentation_helper)

    RESTORE_STACK_FRAME
    retq
    // RESTORE_STACK_ALIGNMENT
    CFI_ENDPROC
    ASM_SIZE(__llvm_mip_call_counts_caller)

#endif // defined(__x86_64__)
