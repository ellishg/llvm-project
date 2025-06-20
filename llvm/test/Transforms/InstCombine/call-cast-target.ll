; NOTE: Assertions have been autogenerated by utils/update_test_checks.py UTC_ARGS: --version 5
; RUN: opt < %s -passes=instcombine -S | FileCheck %s

target datalayout = "e-p:32:32"
target triple = "i686-pc-linux-gnu"

define i32 @main() {
; CHECK-LABEL: define i32 @main() {
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    [[TMP:%.*]] = call ptr @ctime(ptr null)
; CHECK-NEXT:    [[TMP0:%.*]] = ptrtoint ptr [[TMP]] to i32
; CHECK-NEXT:    ret i32 [[TMP0]]
;
entry:
  %tmp = call i32 @ctime( ptr null )          ; <i32> [#uses=1]
  ret i32 %tmp
}

define ptr @ctime(ptr %p) {
; CHECK-LABEL: define ptr @ctime(
; CHECK-SAME: ptr [[P:%.*]]) {
; CHECK-NEXT:    ret ptr [[P]]
;
  ret ptr %p
}

define internal { i8 } @foo(ptr) {
; CHECK-LABEL: define internal { i8 } @foo(
; CHECK-SAME: ptr [[TMP0:%.*]]) {
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    ret { i8 } zeroinitializer
;
entry:
  ret { i8 } { i8 0 }
}

define void @test_struct_ret() {
; CHECK-LABEL: define void @test_struct_ret() {
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    [[TMP0:%.*]] = call { i8 } @foo(ptr null)
; CHECK-NEXT:    ret void
;
entry:
  %0 = call { i8 } @foo(ptr null)
  ret void
}

define i32 @fn1(i32 %x) {
; CHECK-LABEL: define i32 @fn1(
; CHECK-SAME: i32 [[X:%.*]]) {
; CHECK-NEXT:    ret i32 [[X]]
;
  ret i32 %x
}

define i32 @test1(ptr %a) {
; CHECK-LABEL: define i32 @test1(
; CHECK-SAME: ptr [[A:%.*]]) {
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    [[TMP0:%.*]] = ptrtoint ptr [[A]] to i32
; CHECK-NEXT:    [[CALL:%.*]] = tail call i32 @fn1(i32 [[TMP0]])
; CHECK-NEXT:    ret i32 [[CALL]]
;
entry:
  %call = tail call i32 @fn1(ptr %a)
  ret i32 %call
}

declare i32 @fn2(i16)

define i32 @test2(ptr %a) {
; CHECK-LABEL: define i32 @test2(
; CHECK-SAME: ptr [[A:%.*]]) {
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    [[CALL:%.*]] = tail call i32 @fn2(ptr [[A]])
; CHECK-NEXT:    ret i32 [[CALL]]
;
entry:
  %call = tail call i32 @fn2(ptr %a)
  ret i32 %call
}

declare i32 @fn3(i64)

define i32 @test3(ptr %a) {
; CHECK-LABEL: define i32 @test3(
; CHECK-SAME: ptr [[A:%.*]]) {
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    [[CALL:%.*]] = tail call i32 @fn3(ptr [[A]])
; CHECK-NEXT:    ret i32 [[CALL]]
;
entry:
  %call = tail call i32 @fn3(ptr %a)
  ret i32 %call
}

declare i32 @fn4(i32) "thunk"

define i32 @test4(ptr %a) {
; CHECK-LABEL: define i32 @test4(
; CHECK-SAME: ptr [[A:%.*]]) {
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    [[CALL:%.*]] = tail call i32 @fn4(ptr [[A]])
; CHECK-NEXT:    ret i32 [[CALL]]
;
entry:
  %call = tail call i32 @fn4(ptr %a)
  ret i32 %call
}

declare i1 @fn5(ptr byval({ i32, i32 }) align 4 %r)

define i1 @test5(ptr %ptr) {
; CHECK-LABEL: define i1 @test5(ptr %ptr) {
; CHECK-NEXT:    [[TMP2:%.*]] = load i32, ptr [[PTR:%.*]], align 4
; CHECK-NEXT:    [[TMP3:%.*]] = getelementptr inbounds nuw i8, ptr [[PTR]], i32 4
; CHECK-NEXT:    [[TMP4:%.*]] = load i32, ptr [[TMP3]], align 4
; CHECK-NEXT:    [[TMP5:%.*]] = call i1 @fn5(i32 [[TMP2]], i32 [[TMP4]])
; CHECK-NEXT:    ret i1 [[TMP5]]
;
  %2 = getelementptr inbounds { i32, i32 }, ptr %ptr, i32 0, i32 0
  %3 = load i32, ptr %2, align 4
  %4 = getelementptr inbounds { i32, i32 }, ptr %ptr, i32 0, i32 1
  %5 = load i32, ptr %4, align 4
  %6 = call i1 @fn5(i32 %3, i32 %5)
  ret i1 %6
}

define void @bundles_callee(i32) {
; CHECK-LABEL: define void @bundles_callee(
; CHECK-SAME: i32 [[TMP0:%.*]]) {
; CHECK-NEXT:    ret void
;
  ret void
}

define void @bundles() {
; CHECK-LABEL: define void @bundles() {
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    call void @bundles_callee(i32 0) [ "deopt"() ]
; CHECK-NEXT:    ret void
;
entry:
  call void @bundles_callee() [ "deopt"() ]
  ret void
}
