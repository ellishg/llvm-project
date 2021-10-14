; RUN: opt < %s -instrprof -debug-info-correlate -S | FileCheck %s
; RUN: opt < %s -instrprof -debug-info-correlate -S | llc -mtriple=arm64-unknown-linux-gnu | FileCheck %s --check-prefix CHECK-ASM

target triple = "aarch64-unknown-linux-gnu"

@__profn_foo = private constant [3 x i8] c"foo"
; CHECK:      @__profc_foo =
; CHECK-SAME: !dbg ![[EXPR:[0-9]+]]

; CHECK:      ![[EXPR]] = !DIGlobalVariableExpression(var: ![[GLOBAL:[0-9]+]]
; CHECK:      ![[GLOBAL]] = {{.*}} !DIGlobalVariable(name: "__profc_foo"
; CHECK-SAME: scope: ![[SCOPE:[0-9]+]]
; CHECK-SAME: annotations: ![[ANNOTATIONS:[0-9]+]]
; CHECK:      ![[SCOPE]] = {{.*}} !DISubprogram(name: "foo"
; CHECK:      ![[ANNOTATIONS]] = !{![[NAME:[0-9]+]], ![[HASH:[0-9]+]], ![[COUNTERS:[0-9]+]]}
; CHECK:      ![[NAME]] = !{!"Function Name", !"foo"}
; CHECK:      ![[HASH]] = !{!"CFG Hash", !DIExpression(DW_OP_constu, 12345678,
; CHECK:      ![[COUNTERS]] = !{!"Num Counters", !DIExpression(DW_OP_constu, 2,

; CHECK-ASM-NOT: .section   __llvm_prf_data
; CHECK-ASM-NOT: .section   __llvm_prf_names

define void @_Z3foov() !dbg !12 {
  call void @llvm.instrprof.increment(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @__profn_foo, i32 0, i32 0), i64 12345678, i32 2, i32 0)
  ret void
}

declare void @llvm.instrprof.increment(i8*, i64, i32, i32)

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8, !9, !10}
!llvm.ident = !{!11}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 14.0.0", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "debug-info-correlate.cpp", directory: "")
!2 = !{i32 7, !"Dwarf Version", i32 4}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 1, !"branch-target-enforcement", i32 0}
!6 = !{i32 1, !"sign-return-address", i32 0}
!7 = !{i32 1, !"sign-return-address-all", i32 0}
!8 = !{i32 1, !"sign-return-address-with-bkey", i32 0}
!9 = !{i32 7, !"uwtable", i32 1}
!10 = !{i32 7, !"frame-pointer", i32 1}
!11 = !{!"clang version 14.0.0"}
!12 = distinct !DISubprogram(name: "foo", linkageName: "_Z3foov", scope: !13, file: !13, line: 1, type: !14, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !16)
!13 = !DIFile(filename: "debug-info-correlate.cpp", directory: "")
!14 = !DISubroutineType(types: !15)
!15 = !{null}
!16 = !{}
