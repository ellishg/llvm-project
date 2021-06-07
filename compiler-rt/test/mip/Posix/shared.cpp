// RUN: %clang -fmachine-profile-generate -fmachine-profile-function-coverage -g -fPIC -shared -o %t.so %S/../Inputs/external-symbols.cpp
// RUN: llvm-objcopy --remove-section=__llvm_mipmap %t.so %t.stripped.so
// RUN: llvm-objdump --headers %t.stripped.so | FileCheck %s --check-prefix SECTIONS
// RUN: llvm-objcopy --dump-section=__llvm_mipmap=%t.mipmap %t.so
// RUN: %llvm_mipdata create -p %t.mip %t.mipmap
// RUN: %clang -l:%t.so -L/ -o %t.out %s %t.so
//
// RUN: LLVM_MIP_PROFILE_FILENAME=%t.mipraw %run %t.out
// RUN: %llvm_mipdata merge -p %t.mip %t.mipraw
// RUN: %llvm_mipdata covered -p %t.mip --debug %t.so | FileCheck %s
//
// REQUIRES: built-in-llvm-tree

void use_external_symbols();

int main() {
  // CHECK: use_external_symbols()
  use_external_symbols();
  return 0;
}

// SECTIONS-NOT: __llvm_mipmap
