; RUN: llc < %s -enable-machine-instrumentation -enable-machine-function-coverage -mtriple=thumb-linux | FileCheck %s

; CHECK-LABEL: _Z3foov:
define i32 @_Z3foov() #0 {
  ; CHECK:      push    {r0, r1}
  ; CHECK-NEXT: ldr     r1, [[LABEL:.*]]
  ; CHECK-NEXT: [[LOAD_LABEL:.*]]:
  ; CHECK-NEXT: add     r1, pc
  ; CHECK-NEXT: movs    r0, #0
  ; CHECK-NEXT: strb    r0, [r1]
  ; CHECK-NEXT: pop     {r0, r1}
  ; CHECK-NEXT: b       [[CONTINUE:.*]]
  ; CHECK-NEXT: .p2align      2
  ; CHECK-NEXT: [[LABEL]]:
  ; CHECK-NEXT: .long   _Z3foov$RAW-([[LOAD_LABEL]]+4)
  ; CHECK-NEXT: [[CONTINUE]]:

  ret i32 0
  ; CHECK: bx   lr
}

; CHECK-LABEL: .section        __llvm_mipraw,"aGw",%progbits,"_Z3foov$MIP"

; CHECK-LABEL: _Z3foov$RAW:
; CHECK-NEXT: .byte     0xff
