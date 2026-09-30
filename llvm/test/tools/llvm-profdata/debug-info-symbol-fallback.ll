; REQUIRES: aarch64-registered-target

; RUN: split-file %s %t
; RUN: llc -filetype=obj %t/input.ll -o %t.o
; RUN: llvm-dwarfdump %t.o | FileCheck %s --check-prefix=DWARF
; RUN: llvm-profdata show --show-format=yaml --debug-info=%t.o --max-debug-info-correlation-warnings=0 2>&1 | FileCheck %s --implicit-check-not=warning:

; DWARF: DW_AT_name ("bar")
; DWARF: DW_TAG_subprogram
; DWARF-NOT: DW_AT_low_pc
; DWARF-NOT: DW_AT_ranges
; DWARF: DW_TAG_variable
; DWARF: DW_AT_name ("__profc_goo")
; DWARF: DW_AT_location (DW_OP_addr

; CHECK: Probes:
; CHECK: Function Name: foo
; CHECK: Function Name: bar
; CHECK: Function Name: goo

;--- input.c
int foo(void) { return 0; }
int bar(void) { return 1; }
int goo(void) { return 2; }

;--- gen
clang --target=arm64-apple-macosx -g -fno-ident -fdebug-compilation-dir=. -fprofile-generate -mllvm -profile-correlate=debug-info -S -emit-llvm input.c -o input.tmp.ll
# Remove !dbg from the @goo definition to simulate a missing DW_AT_low_pc attribute.
sed '/^define .*@goo()/s/ !dbg ![0-9][0-9]*//' input.tmp.ll

;--- input.ll
; ModuleID = 'input.c'
source_filename = "input.c"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx10.4.0"

@__llvm_profile_raw_version = weak hidden local_unnamed_addr constant i64 648518346341351435
@__llvm_profile_runtime = external hidden local_unnamed_addr global i32
@__profc_foo = internal global [1 x i64] zeroinitializer, section "__DATA,__llvm_prf_cnts", align 8, !dbg !0
@__profc_bar = internal global [1 x i64] zeroinitializer, section "__DATA,__llvm_prf_cnts", align 8, !dbg !9
@__profc_goo = internal global [1 x i64] zeroinitializer, section "__DATA,__llvm_prf_cnts", align 8, !dbg !17
@__llvm_profile_filename = weak hidden local_unnamed_addr constant [20 x i8] c"default_%m.proflite\00"
@llvm.compiler.used = appending global [4 x ptr] [ptr @__llvm_profile_runtime_user, ptr @__profc_bar, ptr @__profc_foo, ptr @__profc_goo], section "llvm.metadata"

; Function Attrs: mustprogress nofree norecurse nosync nounwind ssp willreturn memory(readwrite, argmem: none, inaccessiblemem: none) uwtable(sync)
define noundef i32 @foo() local_unnamed_addr #0 !dbg !2 {
entry:
  %pgocount = load i64, ptr @__profc_foo, align 8, !dbg !30
  %0 = add i64 %pgocount, 1, !dbg !30
  store i64 %0, ptr @__profc_foo, align 8, !dbg !30
  ret i32 0, !dbg !30
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind ssp willreturn memory(readwrite, argmem: none, inaccessiblemem: none) uwtable(sync)
define noundef i32 @bar() local_unnamed_addr #0 !dbg !11 {
entry:
  %pgocount = load i64, ptr @__profc_bar, align 8, !dbg !31
  %0 = add i64 %pgocount, 1, !dbg !31
  store i64 %0, ptr @__profc_bar, align 8, !dbg !31
  ret i32 1, !dbg !31
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind ssp willreturn memory(readwrite, argmem: none, inaccessiblemem: none) uwtable(sync)
define noundef i32 @goo() local_unnamed_addr #0 {
entry:
  %pgocount = load i64, ptr @__profc_goo, align 8, !dbg !32
  %0 = add i64 %pgocount, 1, !dbg !32
  store i64 %0, ptr @__profc_goo, align 8, !dbg !32
  ret i32 2, !dbg !32
}

; Function Attrs: noinline
define linkonce_odr hidden i32 @__llvm_profile_runtime_user() #1 {
  %1 = load i32, ptr @__llvm_profile_runtime, align 4
  ret i32 %1
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind ssp willreturn memory(readwrite, argmem: none, inaccessiblemem: none) uwtable(sync) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+altnzcv,+ccdp,+ccidx,+ccpp,+complxnum,+crc,+dit,+dotprod,+flagm,+fp-armv8,+fp16fml,+fptoint,+fullfp16,+jsconv,+lse,+neon,+pauth,+perfmon,+predres,+ras,+rcpc,+rdm,+sb,+sha2,+sha3,+specrestrict,+ssbs,+v8.1a,+v8.2a,+v8.3a,+v8.4a,+v8a" }
attributes #1 = { noinline }

!llvm.dbg.cu = !{!7}
!llvm.module.flags = !{!24, !25, !26, !27, !28, !29}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "__profc_foo", scope: !2, file: !3, type: !12, isLocal: true, isDefinition: true, annotations: !22)
!2 = distinct !DISubprogram(name: "foo", scope: !3, file: !3, line: 1, type: !4, scopeLine: 1, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !7)
!3 = !DIFile(filename: "input.c", directory: ".")
!4 = !DISubroutineType(types: !5)
!5 = !{!6}
!6 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!7 = distinct !DICompileUnit(language: DW_LANG_C11, file: !3, isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, globals: !8, splitDebugInlining: false, nameTableKind: Apple, sysroot: "/")
!8 = !{!0, !9, !17}
!9 = !DIGlobalVariableExpression(var: !10, expr: !DIExpression())
!10 = distinct !DIGlobalVariable(name: "__profc_bar", scope: !11, file: !3, type: !12, isLocal: true, isDefinition: true, annotations: !13)
!11 = distinct !DISubprogram(name: "bar", scope: !3, file: !3, line: 2, type: !4, scopeLine: 2, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !7)
!12 = !DIBasicType(tag: DW_TAG_unspecified_type, name: "Profile Data Type")
!13 = !{!14, !15, !16}
!14 = !{!"Function Name", !"bar"}
!15 = !{!"CFG Hash", i64 742261418966908927}
!16 = !{!"Num Counters", i32 1}
!17 = !DIGlobalVariableExpression(var: !18, expr: !DIExpression())
!18 = distinct !DIGlobalVariable(name: "__profc_goo", scope: !19, file: !3, type: !12, isLocal: true, isDefinition: true, annotations: !20)
!19 = distinct !DISubprogram(name: "goo", scope: !3, file: !3, line: 3, type: !4, scopeLine: 3, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !7)
!20 = !{!21, !15, !16}
!21 = !{!"Function Name", !"goo"}
!22 = !{!23, !15, !16}
!23 = !{!"Function Name", !"foo"}
!24 = !{i32 7, !"Dwarf Version", i32 4}
!25 = !{i32 2, !"Debug Info Version", i32 3}
!26 = !{i32 1, !"wchar_size", i32 4}
!27 = !{i32 8, !"PIC Level", i32 2}
!28 = !{i32 7, !"uwtable", i32 1}
!29 = !{i32 7, !"frame-pointer", i32 1}
!30 = !DILocation(line: 1, column: 17, scope: !2)
!31 = !DILocation(line: 2, column: 17, scope: !11)
!32 = !DILocation(line: 3, column: 17, scope: !19)
