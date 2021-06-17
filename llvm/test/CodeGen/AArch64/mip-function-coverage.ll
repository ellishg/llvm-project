; RUN: llc < %s -enable-machine-instrumentation -enable-machine-function-coverage -mtriple=arm64-linux | FileCheck %s --check-prefixes CHECK,CHECK-ELF
; RUN: llc < %s -enable-machine-instrumentation -enable-machine-function-coverage -mtriple=arm64-apple-ios | FileCheck %s --check-prefixes CHECK,CHECK-MACHO

; CHECK-ELF-LABEL: _Z3foov:
; CHECK-MACHO-LABEL: __Z3foov:
define i32 @_Z3foov() #0 {
  ; CHECK-ELF:      adrp    [[r:x[0-9]+]], _Z3foov$RAW
  ; CHECK-ELF-NEXT: strb    wzr, {{\[}}[[r]], :lo12:_Z3foov$RAW]

  ; CHECK-MACHO:      adrp    [[r:x[0-9]+]], __Z3foov$RAW@PAGE
  ; CHECK-MACHO-NEXT: strb    wzr, {{\[}}[[r]], __Z3foov$RAW@PAGEOFF]
  ret i32 0
  ; CHECK: ret
}

; CHECK-ELF-LABEL: .section        __llvm_mipraw,"aGw",@progbits,"_Z3foov$MIP"
; CHECK-MACHO-LABEL: .section        __DATA,__llvm_mipraw

; CHECK-ELF-LABEL: _Z3foov$RAW:
; CHECK-MACHO-LABEL: __Z3foov$RAW:
; CHECK-NEXT: .byte     0xff
