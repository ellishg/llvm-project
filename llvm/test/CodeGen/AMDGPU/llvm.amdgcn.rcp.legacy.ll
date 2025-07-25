; RUN: llc -mtriple=amdgcn < %s | FileCheck -check-prefix=GCN %s
; RUN: not llc -mtriple=amdgcn -mcpu=fiji < %s 2>&1 | FileCheck -check-prefix=ERROR %s

; ERROR: error: <unknown>:0:0: in function rcp_legacy_f32 void (ptr addrspace(1), float): intrinsic not supported on subtarget

declare float @llvm.amdgcn.rcp.legacy(float) #0

; GCN-LABEL: {{^}}rcp_legacy_f32:
; GCN: v_rcp_legacy_f32_e32 {{v[0-9]+}}, {{s[0-9]+}}
define amdgpu_kernel void @rcp_legacy_f32(ptr addrspace(1) %out, float %src) #1 {
  %rcp = call float @llvm.amdgcn.rcp.legacy(float %src) #0
  store float %rcp, ptr addrspace(1) %out, align 4
  ret void
}

; TODO: Really these should be constant folded
; GCN-LABEL: {{^}}rcp_legacy_f32_constant_4.0
; GCN: v_rcp_legacy_f32_e32 {{v[0-9]+}}, 4.0
define amdgpu_kernel void @rcp_legacy_f32_constant_4.0(ptr addrspace(1) %out) #1 {
  %rcp = call float @llvm.amdgcn.rcp.legacy(float 4.0) #0
  store float %rcp, ptr addrspace(1) %out, align 4
  ret void
}

; GCN-LABEL: {{^}}rcp_legacy_f32_constant_100.0
; GCN: v_rcp_legacy_f32_e32 {{v[0-9]+}}, 0x42c80000
define amdgpu_kernel void @rcp_legacy_f32_constant_100.0(ptr addrspace(1) %out) #1 {
  %rcp = call float @llvm.amdgcn.rcp.legacy(float 100.0) #0
  store float %rcp, ptr addrspace(1) %out, align 4
  ret void
}

; GCN-LABEL: {{^}}rcp_legacy_undef_f32:
; GCN-NOT: v_rcp_legacy_f32
define amdgpu_kernel void @rcp_legacy_undef_f32(ptr addrspace(1) %out) #1 {
  %rcp = call float @llvm.amdgcn.rcp.legacy(float poison)
  store float %rcp, ptr addrspace(1) %out, align 4
  ret void
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind }
