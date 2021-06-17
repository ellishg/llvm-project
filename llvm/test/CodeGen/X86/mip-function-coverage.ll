; RUN: llc < %s -enable-machine-instrumentation -enable-machine-function-coverage -mtriple=x86_64-linux | FileCheck %s --check-prefixes CHECK,CHECK-ELF
; RUN: llc < %s -enable-machine-instrumentation -enable-machine-function-coverage -mtriple=x86_64-apple-macosx | FileCheck %s --check-prefixes CHECK,CHECK-MACHO

; CHECK-ELF-LABEL: _Z3foov:
; CHECK-MACHO-LABEL: __Z3foov:
define i32 @_Z3foov() #0 {
  ; CHECK-ELF: movb    $0, _Z3foov$RAW(%rip)
  ; CHECK-MACHO: movb    $0, __Z3foov$RAW(%rip)
  ret i32 0
  ; CHECK: retq
}

; CHECK-ELF-LABEL: .section        __llvm_mipraw,"aGw",@progbits,"_Z3foov$MIP"
; CHECK-MACHO-LABEL: .section        __DATA,__llvm_mipraw

; CHECK-ELF-LABEL:   _Z3foov$RAW:
; CHECK-MACHO-LABEL: __Z3foov$RAW:
; CHECK-NEXT:  .byte  0xff
