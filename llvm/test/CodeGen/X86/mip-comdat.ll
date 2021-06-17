; RUN: llc < %s -enable-machine-instrumentation -enable-machine-function-coverage -mtriple=x86_64-linux | FileCheck %s

; CHECK-DAG: .section .text.foo,"axG",@progbits,foo,comdat
$foo = comdat any
define i32 @foo(i32) comdat {
    ret i32 101
}


; CHECK-DAG: .section   __llvm_mipraw,"aGw",@progbits,foo,comdat

; CHECK-DAG: .section   __llvm_mipmap,"Gw",@progbits,foo,comdat
