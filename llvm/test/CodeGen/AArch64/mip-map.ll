; RUN: llc < %s -enable-machine-instrumentation -enable-machine-function-coverage -enable-machine-block-coverage -mtriple=arm64-linux | FileCheck %s --check-prefixes CHECK,CHECK-ELF
; RUN: llc < %s -enable-machine-instrumentation -enable-machine-function-coverage -enable-machine-block-coverage -mtriple=arm64-apple-ios | FileCheck %s --check-prefixes CHECK,CHECK-MACHO

@global = local_unnamed_addr global i32 4, align 4

; CHECK-LABEL: _Z3fooi:
define i32 @_Z3fooi(i32) {
    ret i32 101
}
; CHECK: [[FOO_END:.?Lmip_func_end[0-9]+]]:

; CHECK-LABEL: _Z3gooi:
define weak i32 @_Z3gooi(i32 %a) local_unnamed_addr #0 {
entry:
  %cmp = icmp sgt i32 %a, 1
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
; CHECK:   [[GOO_BLOCK0:.?LBB.*]]:
  %0 = load i32, i32* @global, align 4
  %call = tail call i32 @_Z3fooi(i32 1) #2
  %add.neg = sub i32 %a, %0
  %sub = sub i32 %add.neg, %call
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
; CHECK:   [[GOO_BLOCK1:.?LBB.*]]:
  %c.0 = phi i32 [ %sub, %if.then ], [ %a, %entry ]
  %mul = mul nsw i32 %c.0, %a
  ret i32 %mul
}
; CHECK: [[GOO_END:.?Lmip_func_end[0-9]+]]:

attributes #0 = { minsize norecurse optsize ssp uwtable "frame-pointer"="non-leaf" }

; CHECK-ELF-LABEL: .section  __llvm_mipmap,"Gw",@progbits,"_Z3fooi$MIP"

; CHECK-ELF:         .p2align  3
; CHECK-ELF-LABEL: _Z3fooi$MAP:
; CHECK-ELF-NEXT:       [[FOO_REF:.*]]:
; CHECK-ELF-NEXT:    .xword  _Z3fooi$RAW-[[FOO_REF]]        // Raw Profile Symbol PC Offset
; CHECK-ELF-NEXT:    .xword  _Z3fooi-[[FOO_REF]]            // Function PC Offset
; CHECK-ELF-NEXT:    .word  [[FOO_END]]-_Z3fooi             // Function Size
; CHECK-ELF-NEXT:    .word  0x0                             // CFG Signature
; CHECK-ELF-NEXT:    .word  0                               // Non-entry Block Count
; CHECK-ELF-NEXT:    .word  7                               // Function Name Length
; CHECK-ELF-NEXT:    .ascii  "_Z3fooi"

; CHECK-ELF-LABEL: .section  __llvm_mipmap,"Gw",@progbits,"_Z3gooi$MIP"

; CHECK-ELF:         .p2align  3
; CHECK-ELF-LABEL: _Z3gooi$MAP:
; CHECK-ELF-NEXT:  [[GOO_REF:.*]]:
; CHECK-ELF-NEXT:    .xword  _Z3gooi$RAW-[[GOO_REF]]        // Raw Profile Symbol PC Offset
; CHECK-ELF-NEXT:    .xword  _Z3gooi-[[GOO_REF]]            // Function PC Offset
; CHECK-ELF-NEXT:    .word  [[GOO_END]]-_Z3gooi             // Function Size
; CHECK-ELF-NEXT:    .word  0x70c9fa27                      // CFG Signature
; CHECK-ELF-NEXT:    .word  2                               // Non-entry Block Count
; CHECK-ELF-NEXT:    .word  [[GOO_BLOCK0]]-_Z3gooi          // Block 0 Offset
; CHECK-ELF-NEXT:    .word  [[GOO_BLOCK1]]-_Z3gooi          // Block 1 Offset
; CHECK-ELF-NEXT:    .word  7                               // Function Name Length
; CHECK-ELF-NEXT:    .ascii  "_Z3gooi"

; CHECK-MACHO-LABEL: .section        __DATA,__llvm_mipmap

; CHECK-MACHO:         .p2align  3
; CHECK-MACHO-LABEL: __Z3fooi$MAP:
; CHECK-MACHO-NEXT: [[FOO_REF:.*]]:
; CHECK-MACHO-NEXT:    .quad  __Z3fooi$RAW-[[FOO_REF]]        ; Raw Profile Symbol PC Offset
; CHECK-MACHO-NEXT:    .quad  __Z3fooi-[[FOO_REF]]            ; Function PC Offset
; CHECK-MACHO-NEXT:    .set   [[FOO_SIZE:L.+]], [[FOO_END]]-__Z3fooi ; Function Size
; CHECK-MACHO-NEXT:    .long  [[FOO_SIZE]]
; CHECK-MACHO-NEXT:    .long  0x0                             ; CFG Signature
; CHECK-MACHO-NEXT:    .long  0                               ; Non-entry Block Count
; CHECK-MACHO-NEXT:    .long  8                               ; Function Name Length
; CHECK-MACHO-NEXT:    .ascii  "__Z3fooi"

; CHECK-MACHO:         .p2align  3
; CHECK-MACHO-LABEL: __Z3gooi$MAP:
; CHECK-MACHO-NEXT: [[GOO_REF:.*]]:
; CHECK-MACHO-NEXT:    .quad  __Z3gooi$RAW-[[GOO_REF]]        ; Raw Profile Symbol PC Offset
; CHECK-MACHO-NEXT:    .quad  __Z3gooi-[[GOO_REF]]            ; Function PC Offset
; CHECK-MACHO-NEXT:    .set   [[GOO_SIZE:L.+]], [[GOO_END]]-__Z3gooi ; Function Size
; CHECK-MACHO-NEXT:    .long  [[GOO_SIZE]]
; CHECK-MACHO-NEXT:    .long  0x70c9fa27                      ; CFG Signature
; CHECK-MACHO-NEXT:    .long  2                               ; Non-entry Block Count
; CHECK-MACHO-NEXT:    .set   [[B0_SIZE:L.+]], [[GOO_BLOCK0]]-__Z3gooi ; Block 0 Offset
; CHECK-MACHO-NEXT:    .long  [[B0_SIZE]]
; CHECK-MACHO-NEXT:    .set   [[B1_SIZE:L.+]], [[GOO_BLOCK1]]-__Z3gooi ; Block 1 Offset
; CHECK-MACHO-NEXT:    .long  [[B1_SIZE]]
; CHECK-MACHO-NEXT:    .long  8                               ; Function Name Length
; CHECK-MACHO-NEXT:    .ascii  "__Z3gooi"
