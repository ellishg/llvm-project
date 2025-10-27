; Test for GEP zero on top of another GEP
; RUN: opt -S -passes=loop-rolling < %s -loop-rolling-ignore-cost | FileCheck %s
; XFAIL: true

%MyStruct = type { [4 x i32], [4 x i32], [4 x i32] }

; Function that takes a pointer to MyStruct
define void @gep_on_gep(ptr %p) {
; CHECK-LABEL: define dso_local void @gep_on_gep(
; CHECK-SAME: ptr noundef [[P:%.*]]) {
; CHECK-NEXT:  [[LOOP_ROLLING_PRE:.*]]:
; CHECK-NEXT:    %array_ptr = getelementptr inbounds %MyStruct, ptr [[P]], i32 0, i32 1
; CHECK-NEXT:    br label %[[LOOP_ROLLING_BODY:.*]]
; CHECK:       [[LOOP_ROLLING_BODY]]:
; CHECK-NEXT:    [[IV:%.*]] = phi i32 [ 0, %[[LOOP_ROLLING_PRE]] ], [ [[IV_NEXT:%.*]], %[[LOOP_ROLLING_BODY]] ]
; CHECK-NEXT:    [[TMP0:%.*]] = mul i32 [[IV]], 1
; CHECK-NEXT:    [[TMP1:%.*]] = add i32 [[TMP0]], 0
; CHECK-NEXT:    [[TMP2:%.*]] = getelementptr i8, ptr [[P]], i32 [[TMP1]]
; CHECK-NEXT:    store i32 -1, ptr [[TMP2]], align 4
; CHECK-NEXT:    [[IV_NEXT]] = add i32 [[IV]], 1
; CHECK-NEXT:    [[EXIT:%.*]] = icmp ult i32 [[IV_NEXT]], 3
; CHECK-NEXT:    br i1 [[EXIT]], label %[[LOOP_ROLLING_BODY]], label %[[LOOP_ROLLING_EXIT:.*]]
; CHECK:       [[LOOP_ROLLING_EXIT]]:
; CHECK-NEXT:    ret void
;
entry:
  %array_ptr = getelementptr inbounds %MyStruct, ptr %p, i32 0, i32 1
  store i32 -1, ptr %array_ptr

  %elem_ptr = getelementptr inbounds [4 x i32], ptr %array_ptr, i32 0, i32 1
  store i32 -1, ptr %elem_ptr

  %elem_ptr1 = getelementptr inbounds [4 x i32], ptr %array_ptr, i32 0, i32 2
  store i32 -1, ptr %elem_ptr1

  ret void
}
