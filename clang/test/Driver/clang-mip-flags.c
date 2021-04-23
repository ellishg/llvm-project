// REQUIRES: clang-driver
// UNSUPPORTED: windows-msvc

// RUN: %clang -### -fmachine-profile-use=/path/to/profile.mip %s 2>&1 | FileCheck %s --check-prefix USE
// USE: "-cc1"
// USE-SAME: "-mllvm" "-machine-profile-use=/path/to/profile.mip"
// USE-SAME: "-mllvm" "-link-unit-name=a.out"

// RUN: %clang -### -fmachine-profile-use=/path/to/profile.mip -o my-executable %s 2>&1 | FileCheck %s --check-prefix USE-OUTPUT
// RUN: %clang -### -fmachine-profile-use=/path/to/profile.mip -fmachine-profile-link-unit-name=my-executable %s 2>&1 | FileCheck %s --check-prefix USE-OUTPUT
// USE-OUTPUT: "-cc1"
// USE-OUTPUT-SAME: "-mllvm" "-link-unit-name=my-executable"

// RUN: %clang -### -fmachine-profile-generate %s 2>&1 | FileCheck %s --check-prefix GEN
// RUN: %clang -### -fno-machine-profile-generate -fmachine-profile-generate %s 2>&1 | FileCheck %s --check-prefix GEN
// GEN: "-cc1"
// GEN-SAME: "-mllvm" "-enable-machine-instrumentation"
// GEN-SAME: "-mllvm" "-link-unit-name=a.out"

// RUN: %clang -### %s 2>&1 | FileCheck %s --check-prefix NOGEN
// RUN: %clang -### -fno-machine-profile-generate %s 2>&1 | FileCheck %s --check-prefix NOGEN
// RUN: %clang -### -fmachine-profile-generate -fno-machine-profile-generate %s 2>&1 | FileCheck %s --check-prefix NOGEN
// NOGEN-NOT: "-enable-machine-instrumentation"

// RUN: %clang -### -fmachine-profile-generate -fmachine-profile-function-coverage %s 2>&1 | FileCheck %s --check-prefix FUNCCOV
// FUNCCOV: "-cc1"
// FUNCCOV-SAME: "-mllvm" "-enable-machine-function-coverage"

// RUN: %clang -### -fmachine-profile-generate -fmachine-profile-block-coverage %s 2>&1 | FileCheck %s --check-prefix BLOCKCOV
// BLOCKCOV: "-cc1"
// BLOCKCOV-SAME: "-mllvm" "-enable-machine-block-coverage"

// RUN: %clang -### -fmachine-profile-generate %s 2>&1 | FileCheck %s --check-prefix FULL
// RUN: %clang -### -fmachine-profile-generate -fmachine-profile-call-graph %s 2>&1 | FileCheck %s --check-prefix FULL
// FULL: "-cc1"
// FULL-SAME: "-mllvm" "-enable-machine-call-graph"

// RUN: %clang -### -fmachine-profile-generate -fmachine-profile-runtime-buffer=1024 %s 2>&1 | FileCheck %s --check-prefix RUNTIMEBUF
// RUNTIMEBUF: "-cc1"
// RUNTIMEBUF-SAME: "-mllvm" "-machine-profile-runtime-buffer=1024"

// RUN: %clang -### -fmachine-profile-generate -fmachine-profile-function-group-count=22 -fmachine-profile-selected-function-group=11 %s 2>&1 | FileCheck %s --check-prefix GEN-GROUPS
// GEN-GROUPS: "-cc1"
// GEN-GROUPS-SAME: "-mllvm" "-machine-profile-function-group-count=22"
// GEN-GROUPS-SAME: "-mllvm" "-machine-profile-selected-function-group=11"

// RUN: %clang -### -fmachine-profile-generate -fmachine-profile-min-instruction-count=50 %s 2>&1 | FileCheck %s --check-prefix MIN-INSTR
// MIN-INSTR: "-cc1"
// MIN-INSTR-SAME: "-mllvm" "-machine-profile-min-instruction-count=50"
