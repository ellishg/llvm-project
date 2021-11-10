; RUN: opt < %s -instrprof -S | FileCheck %s

target triple = "aarch64-unknown-linux-gnu"

@__profn_foo = private constant [3 x i8] c"foo"
; HECK: @__profc_foo = private global [3 x i8] c"\FF\FF\FF", section "__llvm_prf_cnts", comdat, align 1
; CHECK: @__profc_foo = private global [3 x i64] [i64 -1, i64 -1, i64 -1], section "__llvm_prf_cnts", comdat, align 8

define void @_Z3foov(i1 %i) {
  call void @llvm.instrprof.cover(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @__profn_foo, i32 0, i32 0), i64 12345678, i32 3, i32 0)
  ; CHECK: store i64 0, i64* getelementptr inbounds ([3 x i64], [3 x i64]* @__profc_foo, i64 0, i64 0), align 4
  %index = select i1 %i, i32 1, i32 2
  call void @llvm.instrprof.cover(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @__profn_foo, i32 0, i32 0), i64 12345678, i32 3, i32 %index)
  ; CHECK: [[ADDRESS:%.*]] = getelementptr inbounds [3 x i64], [3 x i64]* @__profc_foo, i64 0, i32 %index
  ; CHECK: store i64 0, i64* [[ADDRESS]], align 4
  ret void
}

declare void @llvm.instrprof.cover(i8*, i64, i32, i32)
