// RUN: %clang -fmachine-profile-generate -ffunction-sections -fuse-ld=lld -Wl,--icf=all -o %t.out %s
// RUN: llvm-objdump -d %t.out | FileCheck %s
//
// REQUIRES: built-in-llvm-tree, lld-available
//
// TODO: These two functions cannot be folded because they reference different
//       raw symbols, but would be folded otherwise.
// XFAIL: *

// CHECK: <_Z3fooi>:
__attribute((noinline)) int foo(int a) { return 4 * a + 1; }
// CHECK-NOT: <_Z3bari>:
__attribute((noinline)) int bar(int a) { return 4 * a + 1; }

// CHECK: <main>:
int main() { return foo(1) + bar(2); }
