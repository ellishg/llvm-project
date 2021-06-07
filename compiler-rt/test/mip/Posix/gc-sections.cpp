// RUN: %clang -fmachine-profile-generate -g -ffunction-sections -fuse-ld=lld -Wl,--gc-sections -o %t.out %s
// RUN: llvm-objcopy --dump-section=__llvm_mipmap=%t.mipmap %t.out
// RUN: %llvm_mipdata create -p %t.mip %t.mipmap
//
// RUN: LLVM_MIP_PROFILE_FILENAME=%t.mipraw %run %t.out
// RUN: %llvm_mipdata merge -p %t.mip %t.mipraw
//
// RUN: llvm-nm %t.out | FileCheck %s --implicit-check-not bar
//
// REQUIRES: built-in-llvm-tree, lld-available

void foo() {}
void bar() {}

int main() {
  foo();
  return 0;
}

// CHECK: _Z3foov
// CHECK: _Z3foov$MAP
// CHECK: _Z3foov$RAW
