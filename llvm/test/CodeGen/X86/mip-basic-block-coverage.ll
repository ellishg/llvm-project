; RUN: llc < %s -enable-machine-instrumentation -enable-machine-function-coverage -enable-machine-block-coverage -mtriple=x86_64-LINUX | FileCheck %s --check-prefixes CHECK,CHECK-ELF
; RUN: llc < %s -enable-machine-instrumentation -enable-machine-function-coverage -enable-machine-block-coverage -mtriple=x86_64-apple-macosx | FileCheck %s --check-prefixes CHECK,CHECK-MACHO

; CHECK-ELF-LABEL:   _Z3fooi:
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

for.cond:                                         ; preds = %for.inc, %entry
  ; CHECK-ELF:   movb    $0, _Z3fooi$RAW+1(%rip)
  ; CHECK-MACHO: movb    $0, __Z3fooi$RAW+1(%rip)
  %0 = load i32, i32* %i, align 4
  %1 = load i32, i32* %a.addr, align 4
  %cmp = icmp slt i32 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  ; CHECK-ELF:   movb    $0, _Z3fooi$RAW+2(%rip)
  ; CHECK-MACHO: movb    $0, __Z3fooi$RAW+2(%rip)
  %2 = load i32, i32* %i, align 4
  %3 = load i32, i32* %sum, align 4
  %add = add nsw i32 %3, %2
  store i32 %add, i32* %sum, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  ; CHECK-ELF:   movb    $0, _Z3fooi$RAW+3(%rip)
  ; CHECK-MACHO: movb    $0, __Z3fooi$RAW+3(%rip)
  %4 = load i32, i32* %i, align 4
  %inc = add nsw i32 %4, 1
  store i32 %inc, i32* %i, align 4
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %5 = load i32, i32* %sum, align 4
  ret i32 %5
  ; CHECK: retq
}

; CHECK-ELF-LABEL: .section        __llvm_mipraw,"aGw",@progbits,"_Z3fooi$MIP"
; CHECK-MACHO-LABEL: .section        __DATA,__llvm_mipraw

; CHECK-ELF-LABEL: _Z3fooi$RAW:
; CHECK-ELF-NEXT: .byte     0xff
; CHECK-ELF-NEXT: .zero     3,255

; CHECK-MACHO-LABEL: __Z3fooi$RAW:
; CHECK-MACHO-NEXT: .byte     0xff
; CHECK-MACHO-NEXT: .space    3,255
