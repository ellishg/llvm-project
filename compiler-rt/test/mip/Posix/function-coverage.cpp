// RUN: %clang -fmachine-profile-generate -fmachine-profile-function-coverage -g -o %t.out %s
// RUN: llvm-objcopy --remove-section=__llvm_mipmap %t.out %t.stripped
// RUN: llvm-objdump --headers %t.stripped | FileCheck %s --check-prefix SECTIONS
// RUN: llvm-objcopy --dump-section=__llvm_mipmap=%t.mipmap %t.out
// RUN: %llvm_mipdata create -p %t.mip %t.mipmap
//
// RUN: LLVM_MIP_PROFILE_FILENAME=%t.mipraw %run %t.stripped
// RUN: %llvm_mipdata merge -p %t.mip %t.mipraw
// RUN: %llvm_mipdata covered -p %t.mip --debug %t.out | FileCheck %s --check-prefixes CHECK,CHECK-MISSING
//
// REQUIRES: built-in-llvm-tree

void not_called() {}
// CHECK-DAG: called()
void called() {}

// CHECK-DAG: main
int main() {
  called();
  return 0;
}

// CHECK-MISSING-NOT: not_called()

// SECTIONS-NOT: __llvm_mipmap
