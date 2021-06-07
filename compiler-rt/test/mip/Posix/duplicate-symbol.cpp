// RUN: %clang -fmachine-profile-generate -fmachine-profile-function-coverage -g -o %t.out %s %S/../Inputs/external-symbols.cpp
// RUN: llvm-objcopy --dump-section=__llvm_mipmap=%t.mipmap %t.out
// RUN: %llvm_mipdata create -p %t.mip %t.mipmap
// RUN: %llvm_mipdata show -p %t.mip --debug %t.out -o %t.show
// RUN: FileCheck %s --input-file %t.show
// RUN: FileCheck %S/../Inputs/external-symbols.cpp --input-file %t.show
//
// REQUIRES: built-in-llvm-tree

// CHECK-DAG: {{.*}}/duplicate-symbol.cpp:[[@LINE+1]]
static void duplicate_symbol() {}
extern void use_external_symbols();

int main() {
  (void)use_external_symbols;
  (void)duplicate_symbol;
  return 0;
}
