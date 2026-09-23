// RUN: %clang_pgogen -o %t -g -mllvm --profile-correlate=debug-info %s
// RUN: llvm-profdata show --debug-info=%t --detailed-summary --show-prof-sym-list | FileCheck %s
// RUN: llvm-profdata show --debug-info=%t --show-format=yaml | FileCheck %s --match-full-lines --check-prefix YAML

// RUN: %clang_pgogen -o %t.no.dbg -mllvm --profile-correlate=debug-info %s
// RUN: not llvm-profdata show --debug-info=%t.no.dbg 2>&1 | FileCheck %s --check-prefix NO-DBG
// NO-DBG: unable to correlate profile: could not find any profile data metadata in correlated file

// YAML: Probes:
// YAML:   - Function Name:   a
// YAML:     Linkage Name:    a
// YAML:     CFG Hash:        [[HASH:0x[0-9A-F]+]]
// YAML:     Counter Offset:  0x0
// YAML:     Num Counters:    1
// YAML:     VPInfo Offset:   0x0
// YAML:     File:            [[FILE:'.*']]
// YAML:     Line:            [[@LINE+1]]
void a() {}

// YAML:   - Function Name:   b
// YAML:     Linkage Name:    b
// YAML:     CFG Hash:        [[HASH]]
// YAML:     Counter Offset:  0x8
// YAML:     Num Counters:    1
// YAML:     VPInfo Offset:   0x10
// YAML:     File:            [[FILE]]
// YAML:     Line:            [[@LINE+1]]
void b() {}

// YAML:   - Function Name:   c
// YAML:     Linkage Name:    c
// YAML:     CFG Hash:        0x{{[0-9A-F]+}}
// YAML:     Counter Offset:  0x10
// YAML:     Num Counters:    1
// TODO: Make optional?
// YAML:     VPInfo Offset:   0x20
// YAML:     Bitmap Offset:   0x0
// YAML:     Num BitmapBytes: 0
// YAML:     File:            [[FILE]]
// YAML:     Line:            [[@LINE+1]]
void c(void (*f)(void)) { f(); }

// YAML:   - Function Name:   main
// YAML:     Linkage Name:    main
// YAML:     CFG Hash:        [[HASH]]
// YAML:     Counter Offset:  0x18
// YAML:     Num Counters:    1
// YAML:     VPInfo Offset:   0x30
// YAML:     File:            [[FILE]]
// YAML:     Line:            [[@LINE+1]]
int main() { return 0; }

// CHECK:      a
// CHECK-NEXT: b
// CHECK-NEXT: c
// CHECK-NEXT: main
// CHECK: Counters section size: 0x20 bytes
// CHECK: Found 4 functions
