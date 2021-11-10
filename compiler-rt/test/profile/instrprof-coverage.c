// RUN: %clang_pgogen -mllvm -pgo-coverage-instrumentation %s -o %t.out
// RUN: env LLVM_PROFILE_FILE=%t.profraw %run %t.out
// RUN: llvm-profdata merge -o %t.profdata %t.profraw
// RUN: llvm-profdata show %t.profdata --all-functions --counts | FileCheck %s --check-prefix CHECK-PROF
// RUN: %clang -fprofile-use=%t.profdata -S -emit-llvm %s -o - | FileCheck %s

// CHECK-LABEL: define {{.*}} @goo(
// CHECK-SAME: !prof ![[P0:[0-9]+]]
int goo() { return 5; }

// CHECK-LABEL: define {{.*}} @foo(
// CHECK-SAME: !prof ![[P1:[0-9]+]]
int foo(int i) {
  // CHECK: br i1 %{{.*}}, label %{{.*}}, label %{{.*}}, !prof ![[P2:[0-9]+]]
  if (i)
    return 0;
  return 1;
}

// CHECK-LABEL: define {{.*}} @bar(
// CHECK-SAME: !prof ![[P1]]
int bar(int i) {
  // CHECK: br i1 %{{.*}}, label %{{.*}}, label %{{.*}}, !prof ![[P3:[0-9]+]]
  if (i)
    return 0;
  return 1;
}

// CHECK-LABEL: define {{.*}} @main(
// CHECK-SAME: !prof ![[P1]]
int main() {
  foo(0);
  bar(1);
  if (0)
    goo();

  return 0;
}

// CHECK-DAG: ![[P0]] = !{!"function_entry_count", i64 0}
// CHECK-DAG: ![[P1]] = !{!"function_entry_count", i64 1}
// CHECK-DAG: ![[P2]] = !{!"branch_weights", i32 0, i32 1}
// CHECK-DAG: ![[P3]] = !{!"branch_weights", i32 1, i32 0}

// CHECK-PROF: main
// CHECK-PROF:  Counters: 1
// CHECK-PROF:  Block counts: [1]
// CHECK-PROF: foo:
// CHECK-PROF:   Counters: 4
// CHECK-PROF:   Block counts: [1, 1, 0, 1]
// CHECK-PROF: goo:
// CHECK-PROF:   Counters: 1
// CHECK-PROF:   Block counts: [0]
// CHECK-PROF: bar:
// CHECK-PROF:   Counters: 4
// CHECK-PROF:   Block counts: [1, 1, 1, 0]
