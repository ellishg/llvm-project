; RUN: llc < %s -enable-machine-instrumentation -enable-machine-function-coverage -machine-profile-special-case-list=%S/../Inputs/mip-scl.txt -mtriple=arm64-linux | FileCheck %s
; RUN: llc < %s -enable-machine-instrumentation -enable-machine-function-coverage -machine-profile-special-case-list=%S/../Inputs/mip-scl.txt -mtriple=arm64-apple-ios | FileCheck %s

; CHECK: _Z1ai$RAW:
define i32 @_Z1ai(i32 %i) #0 { ret i32 0 }

; CHECK-NOT: _Z1bv$RAW:
define i32 @_Z1bv(i32 %i) #0 { ret i32 0 }

; CHECK-NOT: _Z1cv$RAW:
define i32 @_Z1cv(i32 %i) #0 { ret i32 0 }
