// RUN: %clang -fmachine-profile-generate -fmachine-profile-block-coverage -g -o %t.out %s
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

bool bar() { return true; }

// CHECK: main
// CHECK-NEXT: Source Info: [[FILE:.*]]:[[@LINE+5]]
// CHECK-NEXT: Call Count: 1
// CHECK-NEXT: Order Sum: 1
// CHECK-NEXT: Block Coverage:
// CHECK-NEXT:   HOT  HOT  COLD HOT
int main() {
  if (bar()) {
    return 0;
  } else {
    return 1;
  }
}

// SECTIONS-NOT: __llvm_mipmap
