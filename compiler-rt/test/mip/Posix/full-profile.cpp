// RUN: %clang -fmachine-profile-generate -g -o %t.out %s
// RUN: llvm-objcopy --remove-section=__llvm_mipmap %t.out %t.stripped
// RUN: llvm-objdump --headers %t.stripped | FileCheck %s --check-prefix SECTIONS
// RUN: llvm-objcopy --dump-section=__llvm_mipmap=%t.mipmap %t.out
// RUN: %llvm_mipdata create -p %t.mip %t.mipmap
//
// RUN: LLVM_MIP_PROFILE_FILENAME=%t.mipraw %run %t.stripped
// RUN: %llvm_mipdata merge -p %t.mip %t.mipraw
// RUN: %llvm_mipdata show -p %t.mip --debug %t.out | FileCheck %s
//
// REQUIRES: built-in-llvm-tree
// TODO: Full MIP instrumentation does not yet support armv7 targets.
// XFAIL: arm

// CHECK: _Z3barv
// CHECK-NEXT: Source Info: [[FILE:.*]]:[[@LINE+3]]
// CHECK-NEXT: Call Count: 2
// CHECK-NEXT: Order Sum: 2
void bar() {}

// CHECK: _Z3foov
// CHECK-NEXT: Source Info: [[FILE]]:[[@LINE+3]]
// CHECK-NEXT: Call Count: 100
// CHECK-NEXT: Order Sum: 3
void foo() {}

int main() {
  bar();
  for (int i = 0; i < 100; i++)
    foo();
  bar();
  return 0;
}

// SECTIONS-NOT: __llvm_mipmap
