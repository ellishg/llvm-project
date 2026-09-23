// RUN: %clang_pgogen -g -mllvm --profile-correlate=debug-info -o %t %s
// RUN: env LLVM_PROFILE_FILE=%t.proflite %run %t
// RUN: llvm-profdata merge -o %t.profdata --debug-info=%t %t.proflite

// RUN: llvm-profdata show --debug-info=%t --show-format=yaml | FileCheck %s --check-prefix=YAML
// RUN: llvm-profdata show --all-functions --ic-targets %t.profdata | FileCheck %s

// YAML: Function Name:   foo
// YAML: VPInfo Offset:   0x0
// YAML: Function Name:   bar
// YAML: VPInfo Offset:   0x20
// YAML: Function Name:   main
// YAML: VPInfo Offset:   0x30

// CHECK: main:
// CHECK:   Indirect Call Site Count: 1
// CHECK:     [  0, foo,          2 ]
// CHECK:     [  0, bar,          2 ]
// CHECK:     [  0,    ,          1 ]
// CHECK: foo:
// CHECK:   Indirect Call Site Count: 0
// CHECK: bar:
// CHECK:   Indirect Call Site Count: 0

typedef int (*FP)(int);

int foo(int a) { return a + 1; }

__attribute__((nodebug)) int missing(int a) { return a; }

int bar(int a) { return a - 1; }

FP Fps[] = {foo, bar, missing};

int main(void) {
  for (int i = 0; i < 5; ++i)
    Fps[i % 3](i);
  return 0;
}

// Keep an uncorrelated descriptor at the end of the vinfo section.
__attribute__((nodebug)) int trailing(int a) { return a + 2; }
