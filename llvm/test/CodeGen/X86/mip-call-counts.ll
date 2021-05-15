; RUN: llc < %s -enable-machine-instrumentation -enable-machine-call-graph -mtriple=x86_64-linux | FileCheck %s --check-prefixes CHECK,CHECK-ELF
; RUN: llc < %s -enable-machine-instrumentation -enable-machine-call-graph -mtriple=x86_64-apple-macosx | FileCheck %s --check-prefixes CHECK,CHECK-MACHO

; CHECK-ELF-LABEL: _Z3foov:
; CHECK-MACHO-LABEL: __Z3foov:
define i32 @_Z3foov() #0 {
  ; CHECK-ELF:      leaq    _Z3foov$RAW(%rip), %rax
  ; CHECK-ELF-NEXT: callq   __llvm_mip_call_counts_caller
  ; CHECK-MACHO:      leaq    __Z3foov$RAW(%rip), %rax
  ; CHECK-MACHO-NEXT: callq   ___llvm_mip_call_counts_caller
  ret i32 0
  ; CHECK: retq
}

; CHECK-ELF-LABEL: .section        __llvm_mipraw,"aGw",@progbits,"_Z3foov$MIP"
; CHECK-MACHO-LABEL: .section        __DATA,__llvm_mipraw

; CHECK:  .p2align   2
; CHECK-ELF-LABEL: _Z3foov$RAW:
; CHECK-MACHO-LABEL: __Z3foov$RAW:

; CHECK-NEXT:  .long      0xffffffff
; CHECK-NEXT:  .long      0xffffffff
