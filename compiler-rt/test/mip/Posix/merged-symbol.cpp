// RUN: %clang -fmachine-profile-generate -fmachine-profile-function-coverage -g -o %t.out %s %S/../Inputs/external-symbols.cpp
// RUN: llvm-objcopy --dump-section=__llvm_mipmap=%t.mipmap %t.out
// RUN: %llvm_mipdata create -p %t.mip %t.mipmap
// RUN: %llvm_mipdata show -p %t.mip | FileCheck %s
//
// REQUIRES: built-in-llvm-tree

// CHECK-DAG: _Z20use_external_symbolsv
inline void merged_symbol() {}
extern void use_external_symbols();

int main() {
  (void)use_external_symbols;
  (void)merged_symbol;
  return 0;
}
