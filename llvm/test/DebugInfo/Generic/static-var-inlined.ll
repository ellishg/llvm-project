; RUN: %llc_dwarf -O0 -filetype=obj < %s | llvm-dwarfdump -v -debug-info - | FileCheck --implicit-check-not "{{DW_TAG|NULL}}" %s

; CHECK: DW_TAG_compile_unit
; CHECK:   DW_TAG_base_type
; CHECK:   DW_TAG_imported_declaration
; CHECK:   DW_TAG_subprogram
; CHECK:     DW_AT_name {{.*}} "foo"
; CHECK:     DW_AT_inline [DW_FORM_data1]    (DW_INL_inlined)
; CHECK:     DW_TAG_variable
; CHECK:       DW_AT_name {{.*}} "local_var"
; CHECK:     DW_TAG_variable
; CHECK:       DW_AT_name {{.*}} "static_var"
; CHECK:     DW_TAG_variable
; CHECK:       DW_AT_name {{.*}} "imported_static_var"
; CHECK:     NULL
; CHECK:   DW_TAG_subprogram
; CHECK:     DW_AT_name {{.*}} "main"
; CHECK:     DW_TAG_inlined_subroutine
; CHECK:       DW_AT_abstract_origin {{.*}} "foo"
; CHECK:       DW_TAG_variable
; CHECK:         DW_AT_abstract_origin {{.*}} "local_var"
; FIXME: The static variable should also be found in the subroutine.
; FIXME:       DW_TAG_variable
; FIXME:         DW_AT_abstract_origin {{.*}} "static_var"
; CHECK:     NULL
; CHECK:   NULL
; CHECK: NULL

@static_var = internal global i32 4, align 4, !dbg !0

define i32 @main() !dbg !7 {
  store i32 0, i32* @static_var, align 4, !dbg !15
  %local_var = alloca i32, align 4
  call void @llvm.dbg.declare(metadata i32* %local_var, metadata !2, metadata !DIExpression()), !dbg !15
  store i32 0, i32* %local_var, align 4, !dbg !15
  ret i32 0
}

declare void @llvm.dbg.declare(metadata, metadata, metadata)

!llvm.dbg.cu = !{!4}
!llvm.module.flags = !{!11, !12, !13}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = !DIGlobalVariable(name: "static_var", scope: !6, file: !5, line: 3, type: !10, isLocal: true, isDefinition: true)
!2 = !DILocalVariable(name: "local_var", scope: !6, file: !5, line: 4, type: !10)
!3 = !DISubroutineType(types: !8)
!4 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !5, emissionKind: FullDebug, globals: !9, splitDebugInlining: false, imports: !17)
!5 = !DIFile(filename: "test.cpp", directory: "/")
!6 = distinct !DISubprogram(name: "foo", scope: !5, file: !5, line: 2, type: !3, scopeLine: 2, unit: !4)
!7 = distinct !DISubprogram(name: "main", scope: !5, file: !5, line: 7, type: !3, scopeLine: 7, unit: !4)
!8 = !{}
!9 = !{!0}
!10 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!11 = !{i32 7, !"Dwarf Version", i32 4}
!12 = !{i32 2, !"Debug Info Version", i32 3}
!13 = !{i32 1, !"wchar_size", i32 4}
!15 = !DILocation(line: 4, column: 7, scope: !6, inlinedAt: !16)
!16 = !DILocation(line: 9, column: 3, scope: !7)
!17 = !{!18}
!18 = !DIImportedEntity(tag: DW_TAG_imported_declaration, scope: !4, entity: !19, line: 122)
!19 = !DIGlobalVariable(name: "imported_static_var", scope: !6, file: !5, line: 3, type: !10, isLocal: true, isDefinition: true)
