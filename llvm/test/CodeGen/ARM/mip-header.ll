; RUN: llc < %s -enable-machine-instrumentation -enable-machine-function-coverage -mtriple=armv7-linux | FileCheck %s

define i32 @_Z3fooii(i32 %a, i32 %b) #0 {
  ret i32 0
}

;================================= .mipraw Header =====================================;
; CHECK:       .section __llvm_mipraw,"aGwR",%progbits,__llvm_mipraw_header,comdat
; CHECK:       .p2align 2
; CHECK:       .long   0x50494dfb                      @ Magic
; CHECK-NEXT:  .short  8                               @ Version
; CHECK-NEXT:  .short  0x21                            @ File Type
; CHECK-NEXT:  .long   0x1                             @ Profile Type
; CHECK-NEXT:  .long   [[MODULE_HASH:.*]]              @ Module Hash
; CHECK-NEXT:  .zero   4
; CHECK-NEXT:  .zero   4
; CHECK-NEXT:  .zero   4
; CHECK-NEXT:  .long   0x20                            @ Offset To Data

;================================= .mipmap Header =====================================;
; CHECK:       .section __llvm_mipmap,"GwR",%progbits,__llvm_mipmap_header,comdat
; CHECK:       .p2align 2
; CHECK:     [[REF:.Lref.+]]:
; CHECK:       .long   0x50494dfb                      @ Magic
; CHECK-NEXT:  .short  8                               @ Version
; CHECK-NEXT:  .short  0x24                            @ File Type
; CHECK-NEXT:  .long   0x1                             @ Profile Type
; CHECK-NEXT:  .long   [[MODULE_HASH:.*]]              @ Module Hash
; CHECK-NEXT:  .long   __start___llvm_mipraw-[[REF]]   @ Raw Section Start PC Offset
; CHECK-NEXT:  .zero   4
; CHECK-NEXT:  .zero   4
; CHECK-NEXT:  .long   0x20                            @ Offset To Data
