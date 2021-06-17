; RUN: llc < %s -enable-machine-instrumentation -enable-machine-function-coverage -mtriple=armv7-linux | FileCheck %s

@global = local_unnamed_addr global i32 4, align 4

; CHECK-LABEL: _Z3fooi:
define i32 @_Z3fooi(i32) {
    ret i32 101
}
; CHECK: [[FOO_END:.Lmip_func_end[0-9]+]]:

; CHECK-LABEL: _Z3gooi:
define weak i32 @_Z3gooi(i32 %a) local_unnamed_addr #0 {
entry:
  %cmp = icmp sgt i32 %a, 1
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %0 = load i32, i32* @global, align 4
  %call = tail call i32 @_Z3fooi(i32 1) #2
  %add.neg = sub i32 %a, %0
  %sub = sub i32 %add.neg, %call
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %c.0 = phi i32 [ %sub, %if.then ], [ %a, %entry ]
  %mul = mul nsw i32 %c.0, %a
  ret i32 %mul
}
; CHECK: [[GOO_END:.Lmip_func_end[0-9]+]]:

attributes #0 = { minsize norecurse optsize ssp uwtable "frame-pointer"="non-leaf" }

; CHECK-LABEL: .section  __llvm_mipmap,"Gw",%progbits,"_Z3fooi$MIP"

; CHECK:         .p2align  2
; CHECK-LABEL: _Z3fooi$MAP:
; CHECK-NEXT:       [[FOO_REF:.*]]:
; CHECK-NEXT:    .long  _Z3fooi$RAW-[[FOO_REF]]         @ Raw Profile Symbol PC Offset
; CHECK-NEXT:    .long  _Z3fooi-[[FOO_REF]]             @ Function PC Offset
; CHECK-NEXT:    .long  [[FOO_END]]-_Z3fooi             @ Function Size
; CHECK-NEXT:    .long  0x0                             @ CFG Signature
; CHECK-NEXT:    .long  0                               @ Non-entry Block Count
; CHECK-NEXT:    .long  7                               @ Function Name Length
; CHECK-NEXT:    .ascii  "_Z3fooi"

; CHECK-LABEL: .section  __llvm_mipmap,"Gw",%progbits,"_Z3gooi$MIP"

; CHECK:         .p2align  2
; CHECK-LABEL: _Z3gooi$MAP:
; CHECK-NEXT:  [[GOO_REF:.*]]:
; CHECK-NEXT:    .long  _Z3gooi$RAW-[[GOO_REF]]         @ Raw Profile Symbol PC Offset
; CHECK-NEXT:    .long  _Z3gooi-[[GOO_REF]]             @ Function PC Offset
; CHECK-NEXT:    .long  [[GOO_END]]-_Z3gooi             @ Function Size
; CHECK-NEXT:    .long  0x70c9fa27                      @ CFG Signature
; CHECK-NEXT:    .long  0                               @ Non-entry Block Count
; CHECK-NEXT:    .long  7                               @ Function Name Length
; CHECK-NEXT:    .ascii  "_Z3gooi"
