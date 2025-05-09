; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -mtriple=thumbv8.1m.main -mattr=+mve.fp -verify-machineinstrs -o - %s | FileCheck %s

define arm_aapcs_vfpcc <8 x half> @test_vfmaq_f16(<8 x half> %a, <8 x half> %b, <8 x half> %c) {
; CHECK-LABEL: test_vfmaq_f16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vfma.f16 q0, q1, q2
; CHECK-NEXT:    bx lr
entry:
  %0 = tail call <8 x half> @llvm.fma.v8f16(<8 x half> %b, <8 x half> %c, <8 x half> %a)
  ret <8 x half> %0
}

define arm_aapcs_vfpcc <4 x float> @test_vfmaq_f32(<4 x float> %a, <4 x float> %b, <4 x float> %c) {
; CHECK-LABEL: test_vfmaq_f32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vfma.f32 q0, q1, q2
; CHECK-NEXT:    bx lr
entry:
  %0 = tail call <4 x float> @llvm.fma.v4f32(<4 x float> %b, <4 x float> %c, <4 x float> %a)
  ret <4 x float> %0
}

define arm_aapcs_vfpcc <8 x half> @test_vfmaq_n_f16(<8 x half> %a, <8 x half> %b, float %c.coerce) {
; CHECK-LABEL: test_vfmaq_n_f16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmov r0, s8
; CHECK-NEXT:    vfma.f16 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = bitcast float %c.coerce to i32
  %tmp.0.extract.trunc = trunc i32 %0 to i16
  %1 = bitcast i16 %tmp.0.extract.trunc to half
  %.splatinsert = insertelement <8 x half> undef, half %1, i32 0
  %.splat = shufflevector <8 x half> %.splatinsert, <8 x half> undef, <8 x i32> zeroinitializer
  %2 = tail call <8 x half> @llvm.fma.v8f16(<8 x half> %b, <8 x half> %.splat, <8 x half> %a)
  ret <8 x half> %2
}

define arm_aapcs_vfpcc <4 x float> @test_vfmaq_n_f32(<4 x float> %a, <4 x float> %b, float %c) {
; CHECK-LABEL: test_vfmaq_n_f32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmov r0, s8
; CHECK-NEXT:    vfma.f32 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %.splatinsert = insertelement <4 x float> undef, float %c, i32 0
  %.splat = shufflevector <4 x float> %.splatinsert, <4 x float> undef, <4 x i32> zeroinitializer
  %0 = tail call <4 x float> @llvm.fma.v4f32(<4 x float> %b, <4 x float> %.splat, <4 x float> %a)
  ret <4 x float> %0
}

define arm_aapcs_vfpcc <8 x half> @test_vfmasq_n_f16(<8 x half> %a, <8 x half> %b, float %c.coerce) {
; CHECK-LABEL: test_vfmasq_n_f16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmov r0, s8
; CHECK-NEXT:    vfmas.f16 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = bitcast float %c.coerce to i32
  %tmp.0.extract.trunc = trunc i32 %0 to i16
  %1 = bitcast i16 %tmp.0.extract.trunc to half
  %.splatinsert = insertelement <8 x half> undef, half %1, i32 0
  %.splat = shufflevector <8 x half> %.splatinsert, <8 x half> undef, <8 x i32> zeroinitializer
  %2 = tail call <8 x half> @llvm.fma.v8f16(<8 x half> %a, <8 x half> %b, <8 x half> %.splat)
  ret <8 x half> %2
}

define arm_aapcs_vfpcc <4 x float> @test_vfmasq_n_f32(<4 x float> %a, <4 x float> %b, float %c) {
; CHECK-LABEL: test_vfmasq_n_f32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmov r0, s8
; CHECK-NEXT:    vfmas.f32 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %.splatinsert = insertelement <4 x float> undef, float %c, i32 0
  %.splat = shufflevector <4 x float> %.splatinsert, <4 x float> undef, <4 x i32> zeroinitializer
  %0 = tail call <4 x float> @llvm.fma.v4f32(<4 x float> %a, <4 x float> %b, <4 x float> %.splat)
  ret <4 x float> %0
}

define arm_aapcs_vfpcc <8 x half> @test_vfmsq_f16(<8 x half> %a, <8 x half> %b, <8 x half> %c) {
; CHECK-LABEL: test_vfmsq_f16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vfms.f16 q0, q2, q1
; CHECK-NEXT:    bx lr
entry:
  %0 = fneg <8 x half> %c
  %1 = tail call <8 x half> @llvm.fma.v8f16(<8 x half> %b, <8 x half> %0, <8 x half> %a)
  ret <8 x half> %1
}

define arm_aapcs_vfpcc <4 x float> @test_vfmsq_f32(<4 x float> %a, <4 x float> %b, <4 x float> %c) {
; CHECK-LABEL: test_vfmsq_f32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vfms.f32 q0, q2, q1
; CHECK-NEXT:    bx lr
entry:
  %0 = fneg <4 x float> %c
  %1 = tail call <4 x float> @llvm.fma.v4f32(<4 x float> %b, <4 x float> %0, <4 x float> %a)
  ret <4 x float> %1
}

define arm_aapcs_vfpcc <16 x i8> @test_vmlaq_n_s8(<16 x i8> %a, <16 x i8> %b, i8 signext %c) {
; CHECK-LABEL: test_vmlaq_n_s8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmla.i8 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %.splatinsert = insertelement <16 x i8> undef, i8 %c, i32 0
  %.splat = shufflevector <16 x i8> %.splatinsert, <16 x i8> undef, <16 x i32> zeroinitializer
  %0 = mul <16 x i8> %.splat, %b
  %1 = add <16 x i8> %0, %a
  ret <16 x i8> %1
}

define arm_aapcs_vfpcc <8 x i16> @test_vmlaq_n_s16(<8 x i16> %a, <8 x i16> %b, i16 signext %c) {
; CHECK-LABEL: test_vmlaq_n_s16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmla.i16 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %.splatinsert = insertelement <8 x i16> undef, i16 %c, i32 0
  %.splat = shufflevector <8 x i16> %.splatinsert, <8 x i16> undef, <8 x i32> zeroinitializer
  %0 = mul <8 x i16> %.splat, %b
  %1 = add <8 x i16> %0, %a
  ret <8 x i16> %1
}

define arm_aapcs_vfpcc <4 x i32> @test_vmlaq_n_s32(<4 x i32> %a, <4 x i32> %b, i32 %c) {
; CHECK-LABEL: test_vmlaq_n_s32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmla.i32 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %.splatinsert = insertelement <4 x i32> undef, i32 %c, i32 0
  %.splat = shufflevector <4 x i32> %.splatinsert, <4 x i32> undef, <4 x i32> zeroinitializer
  %0 = mul <4 x i32> %.splat, %b
  %1 = add <4 x i32> %0, %a
  ret <4 x i32> %1
}

define arm_aapcs_vfpcc <16 x i8> @test_vmlaq_n_u8(<16 x i8> %a, <16 x i8> %b, i8 zeroext %c) {
; CHECK-LABEL: test_vmlaq_n_u8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmla.i8 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %.splatinsert = insertelement <16 x i8> undef, i8 %c, i32 0
  %.splat = shufflevector <16 x i8> %.splatinsert, <16 x i8> undef, <16 x i32> zeroinitializer
  %0 = mul <16 x i8> %.splat, %b
  %1 = add <16 x i8> %0, %a
  ret <16 x i8> %1
}

define arm_aapcs_vfpcc <8 x i16> @test_vmlaq_n_u16(<8 x i16> %a, <8 x i16> %b, i16 zeroext %c) {
; CHECK-LABEL: test_vmlaq_n_u16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmla.i16 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %.splatinsert = insertelement <8 x i16> undef, i16 %c, i32 0
  %.splat = shufflevector <8 x i16> %.splatinsert, <8 x i16> undef, <8 x i32> zeroinitializer
  %0 = mul <8 x i16> %.splat, %b
  %1 = add <8 x i16> %0, %a
  ret <8 x i16> %1
}

define arm_aapcs_vfpcc <4 x i32> @test_vmlaq_n_u32(<4 x i32> %a, <4 x i32> %b, i32 %c) {
; CHECK-LABEL: test_vmlaq_n_u32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmla.i32 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %.splatinsert = insertelement <4 x i32> undef, i32 %c, i32 0
  %.splat = shufflevector <4 x i32> %.splatinsert, <4 x i32> undef, <4 x i32> zeroinitializer
  %0 = mul <4 x i32> %.splat, %b
  %1 = add <4 x i32> %0, %a
  ret <4 x i32> %1
}

define arm_aapcs_vfpcc <16 x i8> @test_vmlasq_n_s8(<16 x i8> %a, <16 x i8> %b, i8 signext %c) {
; CHECK-LABEL: test_vmlasq_n_s8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmlas.i8 q1, q0, r0
; CHECK-NEXT:    vmov q0, q1
; CHECK-NEXT:    bx lr
entry:
  %0 = mul <16 x i8> %b, %a
  %.splatinsert = insertelement <16 x i8> undef, i8 %c, i32 0
  %.splat = shufflevector <16 x i8> %.splatinsert, <16 x i8> undef, <16 x i32> zeroinitializer
  %1 = add <16 x i8> %.splat, %0
  ret <16 x i8> %1
}

define arm_aapcs_vfpcc <8 x i16> @test_vmlasq_n_s16(<8 x i16> %a, <8 x i16> %b, i16 signext %c) {
; CHECK-LABEL: test_vmlasq_n_s16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmlas.i16 q1, q0, r0
; CHECK-NEXT:    vmov q0, q1
; CHECK-NEXT:    bx lr
entry:
  %0 = mul <8 x i16> %b, %a
  %.splatinsert = insertelement <8 x i16> undef, i16 %c, i32 0
  %.splat = shufflevector <8 x i16> %.splatinsert, <8 x i16> undef, <8 x i32> zeroinitializer
  %1 = add <8 x i16> %.splat, %0
  ret <8 x i16> %1
}

define arm_aapcs_vfpcc <4 x i32> @test_vmlasq_n_s32(<4 x i32> %a, <4 x i32> %b, i32 %c) {
; CHECK-LABEL: test_vmlasq_n_s32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmlas.i32 q1, q0, r0
; CHECK-NEXT:    vmov q0, q1
; CHECK-NEXT:    bx lr
entry:
  %0 = mul <4 x i32> %b, %a
  %.splatinsert = insertelement <4 x i32> undef, i32 %c, i32 0
  %.splat = shufflevector <4 x i32> %.splatinsert, <4 x i32> undef, <4 x i32> zeroinitializer
  %1 = add <4 x i32> %.splat, %0
  ret <4 x i32> %1
}

define arm_aapcs_vfpcc <16 x i8> @test_vmlasq_n_u8(<16 x i8> %a, <16 x i8> %b, i8 zeroext %c) {
; CHECK-LABEL: test_vmlasq_n_u8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmlas.i8 q1, q0, r0
; CHECK-NEXT:    vmov q0, q1
; CHECK-NEXT:    bx lr
entry:
  %0 = mul <16 x i8> %b, %a
  %.splatinsert = insertelement <16 x i8> undef, i8 %c, i32 0
  %.splat = shufflevector <16 x i8> %.splatinsert, <16 x i8> undef, <16 x i32> zeroinitializer
  %1 = add <16 x i8> %.splat, %0
  ret <16 x i8> %1
}

define arm_aapcs_vfpcc <8 x i16> @test_vmlasq_n_u16(<8 x i16> %a, <8 x i16> %b, i16 zeroext %c) {
; CHECK-LABEL: test_vmlasq_n_u16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmlas.i16 q1, q0, r0
; CHECK-NEXT:    vmov q0, q1
; CHECK-NEXT:    bx lr
entry:
  %0 = mul <8 x i16> %b, %a
  %.splatinsert = insertelement <8 x i16> undef, i16 %c, i32 0
  %.splat = shufflevector <8 x i16> %.splatinsert, <8 x i16> undef, <8 x i32> zeroinitializer
  %1 = add <8 x i16> %.splat, %0
  ret <8 x i16> %1
}

define arm_aapcs_vfpcc <4 x i32> @test_vmlasq_n_u32(<4 x i32> %a, <4 x i32> %b, i32 %c) {
; CHECK-LABEL: test_vmlasq_n_u32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmlas.i32 q1, q0, r0
; CHECK-NEXT:    vmov q0, q1
; CHECK-NEXT:    bx lr
entry:
  %0 = mul <4 x i32> %b, %a
  %.splatinsert = insertelement <4 x i32> undef, i32 %c, i32 0
  %.splat = shufflevector <4 x i32> %.splatinsert, <4 x i32> undef, <4 x i32> zeroinitializer
  %1 = add <4 x i32> %.splat, %0
  ret <4 x i32> %1
}

define arm_aapcs_vfpcc <16 x i8> @test_vqdmlahq_n_s8(<16 x i8> %a, <16 x i8> %b, i8 signext %c) {
; CHECK-LABEL: test_vqdmlahq_n_s8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vqdmlah.s8 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i8 %c to i32
  %1 = tail call <16 x i8> @llvm.arm.mve.vqdmlah.v16i8(<16 x i8> %a, <16 x i8> %b, i32 %0)
  ret <16 x i8> %1
}

define arm_aapcs_vfpcc <8 x i16> @test_vqdmlahq_n_s16(<8 x i16> %a, <8 x i16> %b, i16 signext %c) {
; CHECK-LABEL: test_vqdmlahq_n_s16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vqdmlah.s16 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %c to i32
  %1 = tail call <8 x i16> @llvm.arm.mve.vqdmlah.v8i16(<8 x i16> %a, <8 x i16> %b, i32 %0)
  ret <8 x i16> %1
}

define arm_aapcs_vfpcc <4 x i32> @test_vqdmlahq_n_s32(<4 x i32> %a, <4 x i32> %b, i32 %c) {
; CHECK-LABEL: test_vqdmlahq_n_s32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vqdmlah.s32 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = tail call <4 x i32> @llvm.arm.mve.vqdmlah.v4i32(<4 x i32> %a, <4 x i32> %b, i32 %c)
  ret <4 x i32> %0
}

define arm_aapcs_vfpcc <16 x i8> @test_vqdmlashq_n_s8(<16 x i8> %m1, <16 x i8> %m2, i8 signext %add) {
; CHECK-LABEL: test_vqdmlashq_n_s8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vqdmlash.s8 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i8 %add to i32
  %1 = tail call <16 x i8> @llvm.arm.mve.vqdmlash.v16i8(<16 x i8> %m1, <16 x i8> %m2, i32 %0)
  ret <16 x i8> %1
}

define arm_aapcs_vfpcc <8 x i16> @test_vqdmlashq_n_s16(<8 x i16> %m1, <8 x i16> %m2, i16 signext %add) {
; CHECK-LABEL: test_vqdmlashq_n_s16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vqdmlash.s16 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %add to i32
  %1 = tail call <8 x i16> @llvm.arm.mve.vqdmlash.v8i16(<8 x i16> %m1, <8 x i16> %m2, i32 %0)
  ret <8 x i16> %1
}

define arm_aapcs_vfpcc <4 x i32> @test_vqdmlashq_n_s32(<4 x i32> %m1, <4 x i32> %m2, i32 %add) {
; CHECK-LABEL: test_vqdmlashq_n_s32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vqdmlash.s32 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = tail call <4 x i32> @llvm.arm.mve.vqdmlash.v4i32(<4 x i32> %m1, <4 x i32> %m2, i32 %add)
  ret <4 x i32> %0
}

define arm_aapcs_vfpcc <16 x i8> @test_vqrdmlahq_n_s8(<16 x i8> %a, <16 x i8> %b, i8 signext %c) {
; CHECK-LABEL: test_vqrdmlahq_n_s8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vqrdmlah.s8 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i8 %c to i32
  %1 = tail call <16 x i8> @llvm.arm.mve.vqrdmlah.v16i8(<16 x i8> %a, <16 x i8> %b, i32 %0)
  ret <16 x i8> %1
}

define arm_aapcs_vfpcc <8 x i16> @test_vqrdmlahq_n_s16(<8 x i16> %a, <8 x i16> %b, i16 signext %c) {
; CHECK-LABEL: test_vqrdmlahq_n_s16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vqrdmlah.s16 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %c to i32
  %1 = tail call <8 x i16> @llvm.arm.mve.vqrdmlah.v8i16(<8 x i16> %a, <8 x i16> %b, i32 %0)
  ret <8 x i16> %1
}

define arm_aapcs_vfpcc <4 x i32> @test_vqrdmlahq_n_s32(<4 x i32> %a, <4 x i32> %b, i32 %c) {
; CHECK-LABEL: test_vqrdmlahq_n_s32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vqrdmlah.s32 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = tail call <4 x i32> @llvm.arm.mve.vqrdmlah.v4i32(<4 x i32> %a, <4 x i32> %b, i32 %c)
  ret <4 x i32> %0
}

define arm_aapcs_vfpcc <16 x i8> @test_vqrdmlashq_n_s8(<16 x i8> %a, <16 x i8> %b, i8 signext %c) {
; CHECK-LABEL: test_vqrdmlashq_n_s8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vqrdmlash.s8 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i8 %c to i32
  %1 = tail call <16 x i8> @llvm.arm.mve.vqrdmlash.v16i8(<16 x i8> %a, <16 x i8> %b, i32 %0)
  ret <16 x i8> %1
}

define arm_aapcs_vfpcc <8 x i16> @test_vqrdmlashq_n_s16(<8 x i16> %a, <8 x i16> %b, i16 signext %c) {
; CHECK-LABEL: test_vqrdmlashq_n_s16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vqrdmlash.s16 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %c to i32
  %1 = tail call <8 x i16> @llvm.arm.mve.vqrdmlash.v8i16(<8 x i16> %a, <8 x i16> %b, i32 %0)
  ret <8 x i16> %1
}

define arm_aapcs_vfpcc <4 x i32> @test_vqrdmlashq_n_s32(<4 x i32> %a, <4 x i32> %b, i32 %c) {
; CHECK-LABEL: test_vqrdmlashq_n_s32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vqrdmlash.s32 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = tail call <4 x i32> @llvm.arm.mve.vqrdmlash.v4i32(<4 x i32> %a, <4 x i32> %b, i32 %c)
  ret <4 x i32> %0
}

define arm_aapcs_vfpcc <8 x half> @test_vfmaq_m_f16(<8 x half> %a, <8 x half> %b, <8 x half> %c, i16 zeroext %p) {
; CHECK-LABEL: test_vfmaq_m_f16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r0
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vfmat.f16 q0, q1, q2
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %p to i32
  %1 = tail call <8 x i1> @llvm.arm.mve.pred.i2v.v8i1(i32 %0)
  %2 = tail call <8 x half> @llvm.arm.mve.fma.predicated.v8f16.v8i1(<8 x half> %b, <8 x half> %c, <8 x half> %a, <8 x i1> %1)
  ret <8 x half> %2
}

define arm_aapcs_vfpcc <4 x float> @test_vfmaq_m_f32(<4 x float> %a, <4 x float> %b, <4 x float> %c, i16 zeroext %p) {
; CHECK-LABEL: test_vfmaq_m_f32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r0
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vfmat.f32 q0, q1, q2
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %p to i32
  %1 = tail call <4 x i1> @llvm.arm.mve.pred.i2v.v4i1(i32 %0)
  %2 = tail call <4 x float> @llvm.arm.mve.fma.predicated.v4f32.v4i1(<4 x float> %b, <4 x float> %c, <4 x float> %a, <4 x i1> %1)
  ret <4 x float> %2
}

define arm_aapcs_vfpcc <8 x half> @test_vfmaq_m_n_f16(<8 x half> %a, <8 x half> %b, float %c.coerce, i16 zeroext %p) {
; CHECK-LABEL: test_vfmaq_m_n_f16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmov r1, s8
; CHECK-NEXT:    vmsr p0, r0
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vfmat.f16 q0, q1, r1
; CHECK-NEXT:    bx lr
entry:
  %0 = bitcast float %c.coerce to i32
  %tmp.0.extract.trunc = trunc i32 %0 to i16
  %1 = bitcast i16 %tmp.0.extract.trunc to half
  %.splatinsert = insertelement <8 x half> undef, half %1, i32 0
  %.splat = shufflevector <8 x half> %.splatinsert, <8 x half> undef, <8 x i32> zeroinitializer
  %2 = zext i16 %p to i32
  %3 = tail call <8 x i1> @llvm.arm.mve.pred.i2v.v8i1(i32 %2)
  %4 = tail call <8 x half> @llvm.arm.mve.fma.predicated.v8f16.v8i1(<8 x half> %b, <8 x half> %.splat, <8 x half> %a, <8 x i1> %3)
  ret <8 x half> %4
}

define arm_aapcs_vfpcc <4 x float> @test_vfmaq_m_n_f32(<4 x float> %a, <4 x float> %b, float %c, i16 zeroext %p) {
; CHECK-LABEL: test_vfmaq_m_n_f32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmov r1, s8
; CHECK-NEXT:    vmsr p0, r0
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vfmat.f32 q0, q1, r1
; CHECK-NEXT:    bx lr
entry:
  %.splatinsert = insertelement <4 x float> undef, float %c, i32 0
  %.splat = shufflevector <4 x float> %.splatinsert, <4 x float> undef, <4 x i32> zeroinitializer
  %0 = zext i16 %p to i32
  %1 = tail call <4 x i1> @llvm.arm.mve.pred.i2v.v4i1(i32 %0)
  %2 = tail call <4 x float> @llvm.arm.mve.fma.predicated.v4f32.v4i1(<4 x float> %b, <4 x float> %.splat, <4 x float> %a, <4 x i1> %1)
  ret <4 x float> %2
}

define arm_aapcs_vfpcc <8 x half> @test_vfmasq_m_n_f16(<8 x half> %a, <8 x half> %b, float %c.coerce, i16 zeroext %p) {
; CHECK-LABEL: test_vfmasq_m_n_f16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmov r1, s8
; CHECK-NEXT:    vmsr p0, r0
; CHECK-NEXT:    vdup.16 q2, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vfmat.f16 q2, q0, q1
; CHECK-NEXT:    vmov q0, q2
; CHECK-NEXT:    bx lr
entry:
  %0 = bitcast float %c.coerce to i32
  %tmp.0.extract.trunc = trunc i32 %0 to i16
  %1 = bitcast i16 %tmp.0.extract.trunc to half
  %.splatinsert = insertelement <8 x half> undef, half %1, i32 0
  %.splat = shufflevector <8 x half> %.splatinsert, <8 x half> undef, <8 x i32> zeroinitializer
  %2 = zext i16 %p to i32
  %3 = tail call <8 x i1> @llvm.arm.mve.pred.i2v.v8i1(i32 %2)
  %4 = tail call <8 x half> @llvm.arm.mve.fma.predicated.v8f16.v8i1(<8 x half> %a, <8 x half> %b, <8 x half> %.splat, <8 x i1> %3)
  ret <8 x half> %4
}

define arm_aapcs_vfpcc <8 x half> @test_vfmasq_m_n_f16_select(<8 x half> %a, <8 x half> %b, float %c.coerce, i16 zeroext %p) {
; CHECK-LABEL: test_vfmasq_m_n_f16_select:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmov r1, s8
; CHECK-NEXT:    vmsr p0, r0
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vfmast.f16 q0, q1, r1
; CHECK-NEXT:    bx lr
entry:
  %0 = bitcast float %c.coerce to i32
  %tmp.0.extract.trunc = trunc i32 %0 to i16
  %1 = bitcast i16 %tmp.0.extract.trunc to half
  %.splatinsert = insertelement <8 x half> undef, half %1, i32 0
  %.splat = shufflevector <8 x half> %.splatinsert, <8 x half> undef, <8 x i32> zeroinitializer
  %2 = zext i16 %p to i32
  %3 = tail call <8 x i1> @llvm.arm.mve.pred.i2v.v8i1(i32 %2)
  %4 = tail call <8 x half> @llvm.fma.v8f16(<8 x half> %a, <8 x half> %b, <8 x half> %.splat)
  %5 = select <8 x i1> %3, <8 x half> %4, <8 x half> %a
  ret <8 x half> %5
}

define arm_aapcs_vfpcc <4 x float> @test_vfmasq_m_n_f32(<4 x float> %a, <4 x float> %b, float %c, i16 zeroext %p) {
; CHECK-LABEL: test_vfmasq_m_n_f32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmov r1, s8
; CHECK-NEXT:    vmsr p0, r0
; CHECK-NEXT:    vdup.32 q2, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vfmat.f32 q2, q0, q1
; CHECK-NEXT:    vmov q0, q2
; CHECK-NEXT:    bx lr
entry:
  %.splatinsert = insertelement <4 x float> undef, float %c, i32 0
  %.splat = shufflevector <4 x float> %.splatinsert, <4 x float> undef, <4 x i32> zeroinitializer
  %0 = zext i16 %p to i32
  %1 = tail call <4 x i1> @llvm.arm.mve.pred.i2v.v4i1(i32 %0)
  %2 = tail call <4 x float> @llvm.arm.mve.fma.predicated.v4f32.v4i1(<4 x float> %a, <4 x float> %b, <4 x float> %.splat, <4 x i1> %1)
  ret <4 x float> %2
}

define arm_aapcs_vfpcc <4 x float> @test_vfmasq_m_n_f32_select(<4 x float> %a, <4 x float> %b, float %c, i16 zeroext %p) {
; CHECK-LABEL: test_vfmasq_m_n_f32_select:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmov r1, s8
; CHECK-NEXT:    vmsr p0, r0
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vfmast.f32 q0, q1, r1
; CHECK-NEXT:    bx lr
entry:
  %.splatinsert = insertelement <4 x float> undef, float %c, i32 0
  %.splat = shufflevector <4 x float> %.splatinsert, <4 x float> undef, <4 x i32> zeroinitializer
  %0 = zext i16 %p to i32
  %1 = tail call <4 x i1> @llvm.arm.mve.pred.i2v.v4i1(i32 %0)
  %2 = tail call <4 x float> @llvm.fma.v4f32(<4 x float> %a, <4 x float> %b, <4 x float> %.splat)
  %3 = select <4 x i1> %1, <4 x float> %2, <4 x float> %a
  ret <4 x float> %3
}

define arm_aapcs_vfpcc <8 x half> @test_vfmsq_m_f16(<8 x half> %a, <8 x half> %b, <8 x half> %c, i16 zeroext %p) {
; CHECK-LABEL: test_vfmsq_m_f16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r0
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vfmst.f16 q0, q1, q2
; CHECK-NEXT:    bx lr
entry:
  %0 = fneg <8 x half> %c
  %1 = zext i16 %p to i32
  %2 = tail call <8 x i1> @llvm.arm.mve.pred.i2v.v8i1(i32 %1)
  %3 = tail call <8 x half> @llvm.arm.mve.fma.predicated.v8f16.v8i1(<8 x half> %b, <8 x half> %0, <8 x half> %a, <8 x i1> %2)
  ret <8 x half> %3
}

define arm_aapcs_vfpcc <4 x float> @test_vfmsq_m_f32(<4 x float> %a, <4 x float> %b, <4 x float> %c, i16 zeroext %p) {
; CHECK-LABEL: test_vfmsq_m_f32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r0
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vfmst.f32 q0, q1, q2
; CHECK-NEXT:    bx lr
entry:
  %0 = fneg <4 x float> %c
  %1 = zext i16 %p to i32
  %2 = tail call <4 x i1> @llvm.arm.mve.pred.i2v.v4i1(i32 %1)
  %3 = tail call <4 x float> @llvm.arm.mve.fma.predicated.v4f32.v4i1(<4 x float> %b, <4 x float> %0, <4 x float> %a, <4 x i1> %2)
  ret <4 x float> %3
}

define arm_aapcs_vfpcc <16 x i8> @test_vmlaq_m_n_s8(<16 x i8> %a, <16 x i8> %b, i8 signext %c, i16 zeroext %p) {
; CHECK-LABEL: test_vmlaq_m_n_s8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vmlat.i8 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i8 %c to i32
  %1 = zext i16 %p to i32
  %2 = tail call <16 x i1> @llvm.arm.mve.pred.i2v.v16i1(i32 %1)
  %3 = tail call <16 x i8> @llvm.arm.mve.vmla.n.predicated.v16i8.v16i1(<16 x i8> %a, <16 x i8> %b, i32 %0, <16 x i1> %2)
  ret <16 x i8> %3
}

define arm_aapcs_vfpcc <8 x i16> @test_vmlaq_m_n_s16(<8 x i16> %a, <8 x i16> %b, i16 signext %c, i16 zeroext %p) {
; CHECK-LABEL: test_vmlaq_m_n_s16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vmlat.i16 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %c to i32
  %1 = zext i16 %p to i32
  %2 = tail call <8 x i1> @llvm.arm.mve.pred.i2v.v8i1(i32 %1)
  %3 = tail call <8 x i16> @llvm.arm.mve.vmla.n.predicated.v8i16.v8i1(<8 x i16> %a, <8 x i16> %b, i32 %0, <8 x i1> %2)
  ret <8 x i16> %3
}

define arm_aapcs_vfpcc <4 x i32> @test_vmlaq_m_n_s32(<4 x i32> %a, <4 x i32> %b, i32 %c, i16 zeroext %p) {
; CHECK-LABEL: test_vmlaq_m_n_s32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vmlat.i32 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %p to i32
  %1 = tail call <4 x i1> @llvm.arm.mve.pred.i2v.v4i1(i32 %0)
  %2 = tail call <4 x i32> @llvm.arm.mve.vmla.n.predicated.v4i32.v4i1(<4 x i32> %a, <4 x i32> %b, i32 %c, <4 x i1> %1)
  ret <4 x i32> %2
}

define arm_aapcs_vfpcc <16 x i8> @test_vmlaq_m_n_u8(<16 x i8> %a, <16 x i8> %b, i8 zeroext %c, i16 zeroext %p) {
; CHECK-LABEL: test_vmlaq_m_n_u8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vmlat.i8 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i8 %c to i32
  %1 = zext i16 %p to i32
  %2 = tail call <16 x i1> @llvm.arm.mve.pred.i2v.v16i1(i32 %1)
  %3 = tail call <16 x i8> @llvm.arm.mve.vmla.n.predicated.v16i8.v16i1(<16 x i8> %a, <16 x i8> %b, i32 %0, <16 x i1> %2)
  ret <16 x i8> %3
}

define arm_aapcs_vfpcc <8 x i16> @test_vmlaq_m_n_u16(<8 x i16> %a, <8 x i16> %b, i16 zeroext %c, i16 zeroext %p) {
; CHECK-LABEL: test_vmlaq_m_n_u16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vmlat.i16 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %c to i32
  %1 = zext i16 %p to i32
  %2 = tail call <8 x i1> @llvm.arm.mve.pred.i2v.v8i1(i32 %1)
  %3 = tail call <8 x i16> @llvm.arm.mve.vmla.n.predicated.v8i16.v8i1(<8 x i16> %a, <8 x i16> %b, i32 %0, <8 x i1> %2)
  ret <8 x i16> %3
}

define arm_aapcs_vfpcc <4 x i32> @test_vmlaq_m_n_u32(<4 x i32> %a, <4 x i32> %b, i32 %c, i16 zeroext %p) {
; CHECK-LABEL: test_vmlaq_m_n_u32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vmlat.i32 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %p to i32
  %1 = tail call <4 x i1> @llvm.arm.mve.pred.i2v.v4i1(i32 %0)
  %2 = tail call <4 x i32> @llvm.arm.mve.vmla.n.predicated.v4i32.v4i1(<4 x i32> %a, <4 x i32> %b, i32 %c, <4 x i1> %1)
  ret <4 x i32> %2
}

define arm_aapcs_vfpcc <16 x i8> @test_vmlasq_m_n_s8(<16 x i8> %a, <16 x i8> %b, i8 signext %c, i16 zeroext %p) {
; CHECK-LABEL: test_vmlasq_m_n_s8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vmlast.i8 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i8 %c to i32
  %1 = zext i16 %p to i32
  %2 = tail call <16 x i1> @llvm.arm.mve.pred.i2v.v16i1(i32 %1)
  %3 = tail call <16 x i8> @llvm.arm.mve.vmlas.n.predicated.v16i8.v16i1(<16 x i8> %a, <16 x i8> %b, i32 %0, <16 x i1> %2)
  ret <16 x i8> %3
}

define arm_aapcs_vfpcc <8 x i16> @test_vmlasq_m_n_s16(<8 x i16> %a, <8 x i16> %b, i16 signext %c, i16 zeroext %p) {
; CHECK-LABEL: test_vmlasq_m_n_s16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vmlast.i16 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %c to i32
  %1 = zext i16 %p to i32
  %2 = tail call <8 x i1> @llvm.arm.mve.pred.i2v.v8i1(i32 %1)
  %3 = tail call <8 x i16> @llvm.arm.mve.vmlas.n.predicated.v8i16.v8i1(<8 x i16> %a, <8 x i16> %b, i32 %0, <8 x i1> %2)
  ret <8 x i16> %3
}

define arm_aapcs_vfpcc <4 x i32> @test_vmlasq_m_n_s32(<4 x i32> %a, <4 x i32> %b, i32 %c, i16 zeroext %p) {
; CHECK-LABEL: test_vmlasq_m_n_s32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vmlast.i32 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %p to i32
  %1 = tail call <4 x i1> @llvm.arm.mve.pred.i2v.v4i1(i32 %0)
  %2 = tail call <4 x i32> @llvm.arm.mve.vmlas.n.predicated.v4i32.v4i1(<4 x i32> %a, <4 x i32> %b, i32 %c, <4 x i1> %1)
  ret <4 x i32> %2
}

define arm_aapcs_vfpcc <16 x i8> @test_vmlasq_m_n_u8(<16 x i8> %a, <16 x i8> %b, i8 zeroext %c, i16 zeroext %p) {
; CHECK-LABEL: test_vmlasq_m_n_u8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vmlast.i8 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i8 %c to i32
  %1 = zext i16 %p to i32
  %2 = tail call <16 x i1> @llvm.arm.mve.pred.i2v.v16i1(i32 %1)
  %3 = tail call <16 x i8> @llvm.arm.mve.vmlas.n.predicated.v16i8.v16i1(<16 x i8> %a, <16 x i8> %b, i32 %0, <16 x i1> %2)
  ret <16 x i8> %3
}

define arm_aapcs_vfpcc <8 x i16> @test_vmlasq_m_n_u16(<8 x i16> %a, <8 x i16> %b, i16 zeroext %c, i16 zeroext %p) {
; CHECK-LABEL: test_vmlasq_m_n_u16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vmlast.i16 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %c to i32
  %1 = zext i16 %p to i32
  %2 = tail call <8 x i1> @llvm.arm.mve.pred.i2v.v8i1(i32 %1)
  %3 = tail call <8 x i16> @llvm.arm.mve.vmlas.n.predicated.v8i16.v8i1(<8 x i16> %a, <8 x i16> %b, i32 %0, <8 x i1> %2)
  ret <8 x i16> %3
}

define arm_aapcs_vfpcc <4 x i32> @test_vmlasq_m_n_u32(<4 x i32> %a, <4 x i32> %b, i32 %c, i16 zeroext %p) {
; CHECK-LABEL: test_vmlasq_m_n_u32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vmlast.i32 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %p to i32
  %1 = tail call <4 x i1> @llvm.arm.mve.pred.i2v.v4i1(i32 %0)
  %2 = tail call <4 x i32> @llvm.arm.mve.vmlas.n.predicated.v4i32.v4i1(<4 x i32> %a, <4 x i32> %b, i32 %c, <4 x i1> %1)
  ret <4 x i32> %2
}

define arm_aapcs_vfpcc <16 x i8> @test_vqdmlahq_m_n_s8(<16 x i8> %a, <16 x i8> %b, i8 signext %c, i16 zeroext %p) {
; CHECK-LABEL: test_vqdmlahq_m_n_s8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vqdmlaht.s8 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i8 %c to i32
  %1 = zext i16 %p to i32
  %2 = tail call <16 x i1> @llvm.arm.mve.pred.i2v.v16i1(i32 %1)
  %3 = tail call <16 x i8> @llvm.arm.mve.vqdmlah.predicated.v16i8.v16i1(<16 x i8> %a, <16 x i8> %b, i32 %0, <16 x i1> %2)
  ret <16 x i8> %3
}

define arm_aapcs_vfpcc <8 x i16> @test_vqdmlahq_m_n_s16(<8 x i16> %a, <8 x i16> %b, i16 signext %c, i16 zeroext %p) {
; CHECK-LABEL: test_vqdmlahq_m_n_s16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vqdmlaht.s16 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %c to i32
  %1 = zext i16 %p to i32
  %2 = tail call <8 x i1> @llvm.arm.mve.pred.i2v.v8i1(i32 %1)
  %3 = tail call <8 x i16> @llvm.arm.mve.vqdmlah.predicated.v8i16.v8i1(<8 x i16> %a, <8 x i16> %b, i32 %0, <8 x i1> %2)
  ret <8 x i16> %3
}

define arm_aapcs_vfpcc <4 x i32> @test_vqdmlahq_m_n_s32(<4 x i32> %a, <4 x i32> %b, i32 %c, i16 zeroext %p) {
; CHECK-LABEL: test_vqdmlahq_m_n_s32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vqdmlaht.s32 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %p to i32
  %1 = tail call <4 x i1> @llvm.arm.mve.pred.i2v.v4i1(i32 %0)
  %2 = tail call <4 x i32> @llvm.arm.mve.vqdmlah.predicated.v4i32.v4i1(<4 x i32> %a, <4 x i32> %b, i32 %c, <4 x i1> %1)
  ret <4 x i32> %2
}

define arm_aapcs_vfpcc <16 x i8> @test_vqdmlashq_m_n_s8(<16 x i8> %m1, <16 x i8> %m2, i8 signext %add, i16 zeroext %p) {
; CHECK-LABEL: test_vqdmlashq_m_n_s8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vqdmlasht.s8 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i8 %add to i32
  %1 = zext i16 %p to i32
  %2 = tail call <16 x i1> @llvm.arm.mve.pred.i2v.v16i1(i32 %1)
  %3 = tail call <16 x i8> @llvm.arm.mve.vqdmlash.predicated.v16i8.v16i1(<16 x i8> %m1, <16 x i8> %m2, i32 %0, <16 x i1> %2)
  ret <16 x i8> %3
}

define arm_aapcs_vfpcc <8 x i16> @test_vqdmlashq_m_n_s16(<8 x i16> %m1, <8 x i16> %m2, i16 signext %add, i16 zeroext %p) {
; CHECK-LABEL: test_vqdmlashq_m_n_s16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vqdmlasht.s16 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %add to i32
  %1 = zext i16 %p to i32
  %2 = tail call <8 x i1> @llvm.arm.mve.pred.i2v.v8i1(i32 %1)
  %3 = tail call <8 x i16> @llvm.arm.mve.vqdmlash.predicated.v8i16.v8i1(<8 x i16> %m1, <8 x i16> %m2, i32 %0, <8 x i1> %2)
  ret <8 x i16> %3
}

define arm_aapcs_vfpcc <4 x i32> @test_vqdmlashq_m_n_s32(<4 x i32> %m1, <4 x i32> %m2, i32 %add, i16 zeroext %p) {
; CHECK-LABEL: test_vqdmlashq_m_n_s32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vqdmlasht.s32 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %p to i32
  %1 = tail call <4 x i1> @llvm.arm.mve.pred.i2v.v4i1(i32 %0)
  %2 = tail call <4 x i32> @llvm.arm.mve.vqdmlash.predicated.v4i32.v4i1(<4 x i32> %m1, <4 x i32> %m2, i32 %add, <4 x i1> %1)
  ret <4 x i32> %2
}

define arm_aapcs_vfpcc <16 x i8> @test_vqrdmlahq_m_n_s8(<16 x i8> %a, <16 x i8> %b, i8 signext %c, i16 zeroext %p) {
; CHECK-LABEL: test_vqrdmlahq_m_n_s8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vqrdmlaht.s8 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i8 %c to i32
  %1 = zext i16 %p to i32
  %2 = tail call <16 x i1> @llvm.arm.mve.pred.i2v.v16i1(i32 %1)
  %3 = tail call <16 x i8> @llvm.arm.mve.vqrdmlah.predicated.v16i8.v16i1(<16 x i8> %a, <16 x i8> %b, i32 %0, <16 x i1> %2)
  ret <16 x i8> %3
}

define arm_aapcs_vfpcc <8 x i16> @test_vqrdmlahq_m_n_s16(<8 x i16> %a, <8 x i16> %b, i16 signext %c, i16 zeroext %p) {
; CHECK-LABEL: test_vqrdmlahq_m_n_s16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vqrdmlaht.s16 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %c to i32
  %1 = zext i16 %p to i32
  %2 = tail call <8 x i1> @llvm.arm.mve.pred.i2v.v8i1(i32 %1)
  %3 = tail call <8 x i16> @llvm.arm.mve.vqrdmlah.predicated.v8i16.v8i1(<8 x i16> %a, <8 x i16> %b, i32 %0, <8 x i1> %2)
  ret <8 x i16> %3
}

define arm_aapcs_vfpcc <4 x i32> @test_vqrdmlahq_m_n_s32(<4 x i32> %a, <4 x i32> %b, i32 %c, i16 zeroext %p) {
; CHECK-LABEL: test_vqrdmlahq_m_n_s32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vqrdmlaht.s32 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %p to i32
  %1 = tail call <4 x i1> @llvm.arm.mve.pred.i2v.v4i1(i32 %0)
  %2 = tail call <4 x i32> @llvm.arm.mve.vqrdmlah.predicated.v4i32.v4i1(<4 x i32> %a, <4 x i32> %b, i32 %c, <4 x i1> %1)
  ret <4 x i32> %2
}

define arm_aapcs_vfpcc <16 x i8> @test_vqrdmlashq_m_n_s8(<16 x i8> %a, <16 x i8> %b, i8 signext %c, i16 zeroext %p) {
; CHECK-LABEL: test_vqrdmlashq_m_n_s8:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vqrdmlasht.s8 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i8 %c to i32
  %1 = zext i16 %p to i32
  %2 = tail call <16 x i1> @llvm.arm.mve.pred.i2v.v16i1(i32 %1)
  %3 = tail call <16 x i8> @llvm.arm.mve.vqrdmlash.predicated.v16i8.v16i1(<16 x i8> %a, <16 x i8> %b, i32 %0, <16 x i1> %2)
  ret <16 x i8> %3
}

define arm_aapcs_vfpcc <8 x i16> @test_vqrdmlashq_m_n_s16(<8 x i16> %a, <8 x i16> %b, i16 signext %c, i16 zeroext %p) {
; CHECK-LABEL: test_vqrdmlashq_m_n_s16:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vqrdmlasht.s16 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %c to i32
  %1 = zext i16 %p to i32
  %2 = tail call <8 x i1> @llvm.arm.mve.pred.i2v.v8i1(i32 %1)
  %3 = tail call <8 x i16> @llvm.arm.mve.vqrdmlash.predicated.v8i16.v8i1(<8 x i16> %a, <8 x i16> %b, i32 %0, <8 x i1> %2)
  ret <8 x i16> %3
}

define arm_aapcs_vfpcc <4 x i32> @test_vqrdmlashq_m_n_s32(<4 x i32> %a, <4 x i32> %b, i32 %c, i16 zeroext %p) {
; CHECK-LABEL: test_vqrdmlashq_m_n_s32:
; CHECK:       @ %bb.0: @ %entry
; CHECK-NEXT:    vmsr p0, r1
; CHECK-NEXT:    vpst
; CHECK-NEXT:    vqrdmlasht.s32 q0, q1, r0
; CHECK-NEXT:    bx lr
entry:
  %0 = zext i16 %p to i32
  %1 = tail call <4 x i1> @llvm.arm.mve.pred.i2v.v4i1(i32 %0)
  %2 = tail call <4 x i32> @llvm.arm.mve.vqrdmlash.predicated.v4i32.v4i1(<4 x i32> %a, <4 x i32> %b, i32 %c, <4 x i1> %1)
  ret <4 x i32> %2
}

declare <16 x i1> @llvm.arm.mve.pred.i2v.v16i1(i32)
declare <8 x i1> @llvm.arm.mve.pred.i2v.v8i1(i32)
declare <4 x i1> @llvm.arm.mve.pred.i2v.v4i1(i32)

declare <8 x half> @llvm.fma.v8f16(<8 x half>, <8 x half>, <8 x half>)
declare <4 x float> @llvm.fma.v4f32(<4 x float>, <4 x float>, <4 x float>)
declare <8 x half> @llvm.arm.mve.fma.predicated.v8f16.v8i1(<8 x half>, <8 x half>, <8 x half>, <8 x i1>)
declare <4 x float> @llvm.arm.mve.fma.predicated.v4f32.v4i1(<4 x float>, <4 x float>, <4 x float>, <4 x i1>)
declare <16 x i8> @llvm.arm.mve.vmla.n.predicated.v16i8.v16i1(<16 x i8>, <16 x i8>, i32, <16 x i1>)
declare <8 x i16> @llvm.arm.mve.vmla.n.predicated.v8i16.v8i1(<8 x i16>, <8 x i16>, i32, <8 x i1>)
declare <4 x i32> @llvm.arm.mve.vmla.n.predicated.v4i32.v4i1(<4 x i32>, <4 x i32>, i32, <4 x i1>)
declare <16 x i8> @llvm.arm.mve.vmlas.n.predicated.v16i8.v16i1(<16 x i8>, <16 x i8>, i32, <16 x i1>)
declare <8 x i16> @llvm.arm.mve.vmlas.n.predicated.v8i16.v8i1(<8 x i16>, <8 x i16>, i32, <8 x i1>)
declare <4 x i32> @llvm.arm.mve.vmlas.n.predicated.v4i32.v4i1(<4 x i32>, <4 x i32>, i32, <4 x i1>)
declare <16 x i8> @llvm.arm.mve.vqdmlah.v16i8(<16 x i8>, <16 x i8>, i32)
declare <8 x i16> @llvm.arm.mve.vqdmlah.v8i16(<8 x i16>, <8 x i16>, i32)
declare <4 x i32> @llvm.arm.mve.vqdmlah.v4i32(<4 x i32>, <4 x i32>, i32)
declare <16 x i8> @llvm.arm.mve.vqdmlash.v16i8(<16 x i8>, <16 x i8>, i32)
declare <8 x i16> @llvm.arm.mve.vqdmlash.v8i16(<8 x i16>, <8 x i16>, i32)
declare <4 x i32> @llvm.arm.mve.vqdmlash.v4i32(<4 x i32>, <4 x i32>, i32)
declare <16 x i8> @llvm.arm.mve.vqrdmlah.v16i8(<16 x i8>, <16 x i8>, i32)
declare <8 x i16> @llvm.arm.mve.vqrdmlah.v8i16(<8 x i16>, <8 x i16>, i32)
declare <4 x i32> @llvm.arm.mve.vqrdmlah.v4i32(<4 x i32>, <4 x i32>, i32)
declare <16 x i8> @llvm.arm.mve.vqrdmlash.v16i8(<16 x i8>, <16 x i8>, i32)
declare <8 x i16> @llvm.arm.mve.vqrdmlash.v8i16(<8 x i16>, <8 x i16>, i32)
declare <4 x i32> @llvm.arm.mve.vqrdmlash.v4i32(<4 x i32>, <4 x i32>, i32)
declare <16 x i8> @llvm.arm.mve.vqdmlah.predicated.v16i8.v16i1(<16 x i8>, <16 x i8>, i32, <16 x i1>)
declare <8 x i16> @llvm.arm.mve.vqdmlah.predicated.v8i16.v8i1(<8 x i16>, <8 x i16>, i32, <8 x i1>)
declare <4 x i32> @llvm.arm.mve.vqdmlah.predicated.v4i32.v4i1(<4 x i32>, <4 x i32>, i32, <4 x i1>)
declare <16 x i8> @llvm.arm.mve.vqdmlash.predicated.v16i8.v16i1(<16 x i8>, <16 x i8>, i32, <16 x i1>)
declare <8 x i16> @llvm.arm.mve.vqdmlash.predicated.v8i16.v8i1(<8 x i16>, <8 x i16>, i32, <8 x i1>)
declare <4 x i32> @llvm.arm.mve.vqdmlash.predicated.v4i32.v4i1(<4 x i32>, <4 x i32>, i32, <4 x i1>)
declare <16 x i8> @llvm.arm.mve.vqrdmlah.predicated.v16i8.v16i1(<16 x i8>, <16 x i8>, i32, <16 x i1>)
declare <8 x i16> @llvm.arm.mve.vqrdmlah.predicated.v8i16.v8i1(<8 x i16>, <8 x i16>, i32, <8 x i1>)
declare <4 x i32> @llvm.arm.mve.vqrdmlah.predicated.v4i32.v4i1(<4 x i32>, <4 x i32>, i32, <4 x i1>)
declare <16 x i8> @llvm.arm.mve.vqrdmlash.predicated.v16i8.v16i1(<16 x i8>, <16 x i8>, i32, <16 x i1>)
declare <8 x i16> @llvm.arm.mve.vqrdmlash.predicated.v8i16.v8i1(<8 x i16>, <8 x i16>, i32, <8 x i1>)
declare <4 x i32> @llvm.arm.mve.vqrdmlash.predicated.v4i32.v4i1(<4 x i32>, <4 x i32>, i32, <4 x i1>)
