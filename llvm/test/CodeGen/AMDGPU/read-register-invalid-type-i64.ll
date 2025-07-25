; RUN: not --crash llc -mtriple=amdgcn < %s 2>&1 | FileCheck %s

; CHECK: invalid type for register "m0".

declare i64 @llvm.read_register.i64(metadata) #0

define amdgpu_kernel void @test_invalid_read_m0(ptr addrspace(1) %out) #0 {
  %exec = call i64 @llvm.read_register.i64(metadata !0)
  store i64 %exec, ptr addrspace(1) %out
  ret void
}

!0 = !{!"m0"}
