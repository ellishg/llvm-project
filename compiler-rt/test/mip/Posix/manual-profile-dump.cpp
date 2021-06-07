// RUN: %clang -fmachine-profile-generate -fmachine-profile-function-coverage -fno-machine-profile-dump -g -o %t.out %s
// RUN: llvm-objcopy --remove-section=__llvm_mipmap %t.out %t.stripped
// RUN: llvm-objdump --headers %t.stripped | FileCheck %s --check-prefix SECTIONS
// RUN: llvm-objcopy --dump-section=__llvm_mipmap=%t.mipmap %t.out
// RUN: %llvm_mipdata create -p %t.mip %t.mipmap
//
// RUN: LLVM_MIP_PROFILE_FILENAME=%t.missing.mipraw %run %t.stripped %t.mipraw
// RUN: not test -f %t.missing.mipraw
// RUN: %llvm_mipdata merge -p %t.mip %t.mipraw
// RUN: %llvm_mipdata show -p %t.mip --debug %t.out | FileCheck %s
//
// REQUIRES: built-in-llvm-tree

extern "C" {
int __llvm_dump_mip_profile_with_filename(const char *Filename);
}

// CHECK: _Z3foov
// CHECK-NEXT: Source Info: [[FILE:.*]]:[[@LINE+2]]
// CHECK-NEXT: Call Count: 1
void foo() {}

// argv[1] holds the filename to dump.
int main(int argc, const char *argv[]) {
  foo();
  __llvm_dump_mip_profile_with_filename(argv[1]);
  return 0;
}

// SECTIONS-NOT: __llvm_mipmap
