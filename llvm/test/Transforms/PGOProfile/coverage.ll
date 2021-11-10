; RUN: opt < %s -passes=pgo-instr-gen -pgo-coverage-instrumentation -S | FileCheck %s
; UN: llvm-profdata merge %S/Inputs/select1.proftext -o %t.profdata
; UN: opt < %s -passes=pgo-instr-use -pgo-test-profile-file=%t.profdata -pgo-instr-select=true -S | FileCheck %s --check-prefix=USE
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i32 @foo(i32 %i) {
entry:
  ; CHECK: call void @llvm.instrprof.cover({{.*}}, i32 6, i32 1)
  %cmp = icmp sgt i32 %i, 0
  br i1 %cmp, label %if.then, label %if.else

if.then:
  ; CHECK: call void @llvm.instrprof.cover({{.*}}, i32 6, i32 2)
  %add = add nsw i32 %i, 2
  ; CHECK: [[INDEX:%.*]] = select i1 %cmp, i32 4, i32 5
  ; CHECK: call void @llvm.instrprof.cover({{.*}}, i32 6, i32 [[INDEX]])
  %s = select i1 %cmp, i32 %add, i32 0
  br label %if.end

if.else:
  ; CHECK: call void @llvm.instrprof.cover({{.*}}, i32 6, i32 3)
  %sub = sub nsw i32 %i, 2
  br label %if.end

if.end:
  ; CHECK: call void @llvm.instrprof.cover({{.*}} i32 6, i32 0)
  %retv = phi i32 [ %add, %if.then ], [ %sub, %if.else ]
  ret i32 %retv
}
