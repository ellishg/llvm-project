; RUN: llc < %s -enable-machine-instrumentation -enable-machine-call-graph -mtriple=arm64-linux | FileCheck %s --check-prefixes CHECK,CHECK-ELF
; RUN: llc < %s -enable-machine-instrumentation -enable-machine-call-graph -mtriple=arm64-apple-ios | FileCheck %s --check-prefixes CHECK,CHECK-MACHO

; CHECK-ELF-LABEL: _Z3foov:
; CHECK-MACHO-LABEL: __Z3foov:
define i32 @_Z3foov() #0 {
  ; CHECK:      stp     x29, x30, [sp, #-16]!
  ; CHECK-ELF-NEXT: adrp    [[r:x[0-9]+]], _Z3foov$RAW
  ; CHECK-ELF-NEXT: add     [[r]], [[r]], :lo12:_Z3foov$RAW
  ; CHECK-ELF-NEXT: bl      __llvm_mip_call_counts_caller

  ; CHECK-MACHO-NEXT: adrp    [[r:x[0-9]+]], __Z3foov$RAW@PAGE
  ; CHECK-MACHO-NEXT: add     [[r]], [[r]], __Z3foov$RAW@PAGEOFF
  ; CHECK-MACHO-NEXT: bl      ___llvm_mip_call_counts_caller
  ret i32 0
  ; CHECK: ret
}

; CHECK-ELF-LABEL: .section        __llvm_mipraw,"aGw",@progbits,"_Z3foov$MIP"
; CHECK-MACHO-LABEL: .section        __DATA,__llvm_mipraw

; CHECK-ELF:       .p2align 2
; CHECK-ELF-LABEL: _Z3foov$RAW:
; CHECK-ELF-NEXT:  .word    0xffffffff
; CHECK-ELF-NEXT:  .word    0xffffffff

; CHECK-MACHO:       .p2align   2
; CHECK-MACHO-LABEL: __Z3foov$RAW:
; CHECK-MACHO-NEXT:  .long      0xffffffff
; CHECK-MACHO-NEXT:  .long      0xffffffff
