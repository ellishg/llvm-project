// RUN: rm -f %t.profdata
// RUN: %clang_pgogen -o %t -mllvm -pgo-temporal-instrumentation %s
// RUN: env LLVM_PROFILE_FILE=%t.0.profraw %run %t n
// RUN: env LLVM_PROFILE_FILE=%t.1.profraw %run %t y
// RUN: llvm-profdata merge -o %t.profdata %t.0.profraw %t.1.profraw
// RUN: llvm-profdata show --function-traces %t.profdata | FileCheck %s --implicit-check-not=unused

// RUN: rm -f %t.profdata
// RUN: %clang_pgogen -o %t -mllvm -pgo-temporal-instrumentation -mllvm -pgo-block-coverage %s
// RUN: env LLVM_PROFILE_FILE=%t.0.profraw %run %t n
// RUN: env LLVM_PROFILE_FILE=%t.1.profraw %run %t y
// RUN: llvm-profdata merge -o %t.profdata %t.0.profraw %t.1.profraw
// RUN: llvm-profdata show --function-traces %t.profdata | FileCheck %s --implicit-check-not=unused

extern void exit(int);
extern void __llvm_profile_reset_counters();

void a() {}
void b() {}
void unused() { exit(1); }
void c() {}

int main(int argc, const char *argv[]) {
  if (argc != 2)
    unused();
  a();
  b();
  b();
  c();
  if (*argv[1] == 'y')
    __llvm_profile_reset_counters();
  a();
  c();
  b();
  return 0;
}

// CHECK: Function Traces (samples=2 seen=2):
// CHECK:   Trace 0 (count=4):
// CHECK:     main
// CHECK:     a
// CHECK:     b
// CHECK:     c
// CHECK:   Trace 1 (count=3):
// CHECK:     a
// CHECK:     c
// CHECK:     b
