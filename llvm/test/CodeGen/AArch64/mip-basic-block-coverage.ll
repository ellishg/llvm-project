; RUN: llc < %s -enable-machine-instrumentation -enable-machine-function-coverage -enable-machine-block-coverage -mtriple=arm64-linux | FileCheck %s --check-prefixes CHECK,CHECK-ELF
; RUN: llc < %s -enable-machine-instrumentation -enable-machine-function-coverage -enable-machine-block-coverage -mtriple=arm64-apple-ios | FileCheck %s --check-prefixes CHECK,CHECK-MACHO

; CHECK-ELF-LABEL: _Z3fooi:
; CHECK-MACHO-LABEL: __Z3fooi:
define i32 @_Z3fooi(i32 %a) {
entry:
  %a.addr = alloca i32, align 4
  %sum = alloca i32, align 4
  %i = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  store i32 0, i32* %sum, align 4
  store i32 0, i32* %i, align 4
  br label %for.cond

; CHECK-LABEL: LBB0_1:
for.cond:                                         ; preds = %for.inc, %entry
  ; CHECK-ELF:        adrp   [[r:x[0-9]+]], _Z3fooi$RAW
  ; CHECK-MACHO:      adrp   [[r:x[0-9]+]], __Z3fooi$RAW@PAGE
  ; CHECK-NEXT:       add    [[r]], [[r]], #1
  ; CHECK-ELF-NEXT:   strb   wzr, {{\[}}[[r]], :lo12:_Z3fooi$RAW]
  ; CHECK-MACHO-NEXT: strb   wzr, {{\[}}[[r]], __Z3fooi$RAW@PAGEOFF]
  %0 = load i32, i32* %i, align 4
  %1 = load i32, i32* %a.addr, align 4
  %cmp = icmp slt i32 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  ; CHECK-ELF:        adrp   [[r:x[0-9]+]], _Z3fooi$RAW
  ; CHECK-MACHO:      adrp   [[r:x[0-9]+]], __Z3fooi$RAW@PAGE
  ; CHECK-NEXT:       add    [[r]], [[r]], #2
  ; CHECK-ELF-NEXT:   strb   wzr, {{\[}}[[r]], :lo12:_Z3fooi$RAW]
  ; CHECK-MACHO-NEXT: strb   wzr, {{\[}}[[r]], __Z3fooi$RAW@PAGEOFF]
  %2 = load i32, i32* %i, align 4
  %3 = load i32, i32* %sum, align 4
  %add = add nsw i32 %3, %2
  store i32 %add, i32* %sum, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  ; CHECK-ELF:        adrp   [[r:x[0-9]+]], _Z3fooi$RAW
  ; CHECK-MACHO:      adrp   [[r:x[0-9]+]], __Z3fooi$RAW@PAGE
  ; CHECK-NEXT:       add    [[r]], [[r]], #3
  ; CHECK-ELF-NEXT:   strb   wzr, {{\[}}[[r]], :lo12:_Z3fooi$RAW]
  ; CHECK-MACHO-NEXT: strb   wzr, {{\[}}[[r]], __Z3fooi$RAW@PAGEOFF]
  %4 = load i32, i32* %i, align 4
  %inc = add nsw i32 %4, 1
  store i32 %inc, i32* %i, align 4
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %5 = load i32, i32* %sum, align 4
  ret i32 %5
  ; CHECK: ret
}

; CHECK-ELF-LABEL: .section        __llvm_mipraw,"aGw",@progbits,"_Z3fooi$MIP"
; CHECK-MACHO-LABEL: .section        __DATA,__llvm_mipraw

; CHECK-ELF-LABEL: _Z3fooi$RAW:
; CHECK-ELF-NEXT: .byte     0xff
; CHECK-ELF-NEXT: .zero     3,255

; CHECK-MACHO-LABEL: __Z3fooi$RAW:
; CHECK-MACHO-NEXT: .byte     0xff
; CHECK-MACHO-NEXT: .space    3,255
