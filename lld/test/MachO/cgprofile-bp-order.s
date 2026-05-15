# REQUIRES: aarch64

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=arm64-apple-darwin %t/a.s -o %t/a.o
# RUN: llvm-profdata merge %t/a.proftext -o %t/a.profdata

# RUN: %lld -arch arm64 -o %t/a.out %t/a.o --call-graph-profile-sort --irpgo-profile=%t/a.profdata --bp-startup-sort=function --verbose-bp-section-orderer 2>&1 | FileCheck %s
# CHECK: Ordered 2 sections ([[#]] bytes) using balanced partitioning

# RUN: llvm-nm --numeric-sort %t/a.out | FileCheck %s --check-prefix=ORDER
# ORDER: T F
# ORDER: T E
# ORDER: T A
# ORDER: T C
# ORDER: T D
# ORDER: T B
# ORDER: T _main

#--- a.s
.text
.globl _main, A, B, C, D, E, F

_main:
  ret
A:
  ret
B:
  ret
C:
  ret
D:
  ret
E:
  ret
F:
  ret

.subsections_via_symbols

.cg_profile A, B, 10
.cg_profile A, C, 40
.cg_profile B, C, 30
.cg_profile C, D, 90
.cg_profile D, E, 50
.cg_profile E, F, 20

#--- a.proftext
:ir
:temporal_prof_traces
# Num Traces
1
# Trace Stream Size:
1
# Weight
1
F, E

E
# Func Hash:
1111
# Num Counters:
1
# Counter Values:
1

F
# Func Hash:
2222
# Num Counters:
1
# Counter Values:
1
