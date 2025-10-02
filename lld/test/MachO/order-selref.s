; REQUIRES: aarch64
; RUN: rm -rf %t && split-file %s %t

; RUN: llvm-mc -filetype=obj -triple=arm64-apple-darwin %t/stubs.s -o %t/stubs.o
; RUN: llvm-mc -filetype=obj -triple=arm64-apple-darwin %t/a.s -o %t/a.o

; RUN: %lld -dylib -arch arm64 -U _objc_msgSend -objc_selref_section_order -o %t/a %t/stubs.o %t/a.o -order_file %t/orderfile.txt
; RUN: llvm-objdump --macho --section="__TEXT,__objc_methname" %t/a | FileCheck %s --check-prefix=STR
; RUN: llvm-nm --numeric-sort --format=just-symbols %t/a | FileCheck %s

; STR: stub2
; STR: m2
; STR: m1
; STR: m3
; STR: stub1

; m2
; CHECK: _OBJC_SELECTOR_REFERENCES_.3
; m1
; CHECK: _OBJC_SELECTOR_REFERENCES_
; m3
; CHECK: _OBJC_SELECTOR_REFERENCES_.4

;--- orderfile.txt
CSTR;1776570828 # stub2
CSTR;1979598959 # m2
CSTR;1707168413 # m1
CSTR;1910439722 # m3
CSTR;1012752865 # stub1

;--- stubs.s
.text
_main:
  bl  _objc_msgSend$stub1
  bl  _objc_msgSend$stub2
  ret

;--- a.mm
__attribute__((objc_root_class))
@interface Foo
- (void) m1;
- (void) m2;
- (void) m3;
@end

@implementation Foo
- (void)m1 {}
- (void)m2 {}
- (void)m3 {}
@end

void test(Foo *foo) {
  [foo m1];
  [foo m2];
  [foo m3];
}

void *_objc_empty_cache;
void *_objc_empty_vtable;
;--- gen
clang -Oz -target arm64-apple-darwin a.mm -S -o -
;--- a.s
	.build_version macos, 11, 0
	.section	__TEXT,__text,regular,pure_instructions
	.p2align	2                               ; -- Begin function -[Foo m1]
"-[Foo m1]":                            ; @"\01-[Foo m1]"
	.cfi_startproc
; %bb.0:
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function -[Foo m2]
"-[Foo m2]":                            ; @"\01-[Foo m2]"
	.cfi_startproc
; %bb.0:
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function -[Foo m3]
"-[Foo m3]":                            ; @"\01-[Foo m3]"
	.cfi_startproc
; %bb.0:
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	__Z4testP3Foo                   ; -- Begin function _Z4testP3Foo
	.p2align	2
__Z4testP3Foo:                          ; @_Z4testP3Foo
	.cfi_startproc
; %bb.0:
	stp	x20, x19, [sp, #-32]!           ; 16-byte Folded Spill
	stp	x29, x30, [sp, #16]             ; 16-byte Folded Spill
	add	x29, sp, #16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	mov	x19, x0
Lloh0:
	adrp	x8, _OBJC_SELECTOR_REFERENCES_@PAGE
Lloh1:
	ldr	x1, [x8, _OBJC_SELECTOR_REFERENCES_@PAGEOFF]
	bl	_objc_msgSend
Lloh2:
	adrp	x8, _OBJC_SELECTOR_REFERENCES_.3@PAGE
Lloh3:
	ldr	x1, [x8, _OBJC_SELECTOR_REFERENCES_.3@PAGEOFF]
	mov	x0, x19
	bl	_objc_msgSend
Lloh4:
	adrp	x8, _OBJC_SELECTOR_REFERENCES_.4@PAGE
Lloh5:
	ldr	x1, [x8, _OBJC_SELECTOR_REFERENCES_.4@PAGEOFF]
	mov	x0, x19
	ldp	x29, x30, [sp, #16]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp], #32             ; 16-byte Folded Reload
	b	_objc_msgSend
	.loh AdrpLdr	Lloh4, Lloh5
	.loh AdrpLdr	Lloh2, Lloh3
	.loh AdrpLdr	Lloh0, Lloh1
	.cfi_endproc
                                        ; -- End function
	.globl	__objc_empty_vtable             ; @_objc_empty_vtable
.zerofill __DATA,__common,__objc_empty_vtable,8,3
	.section	__DATA,__objc_data
	.globl	_OBJC_CLASS_$_Foo               ; @"OBJC_CLASS_$_Foo"
	.p2align	3, 0x0
_OBJC_CLASS_$_Foo:
	.quad	_OBJC_METACLASS_$_Foo
	.quad	0
	.quad	__objc_empty_cache
	.quad	__objc_empty_vtable
	.quad	__OBJC_CLASS_RO_$_Foo

	.globl	_OBJC_METACLASS_$_Foo           ; @"OBJC_METACLASS_$_Foo"
	.p2align	3, 0x0
_OBJC_METACLASS_$_Foo:
	.quad	_OBJC_METACLASS_$_Foo
	.quad	_OBJC_CLASS_$_Foo
	.quad	__objc_empty_cache
	.quad	__objc_empty_vtable
	.quad	__OBJC_METACLASS_RO_$_Foo

	.section	__TEXT,__objc_classname,cstring_literals
l_OBJC_CLASS_NAME_:                     ; @OBJC_CLASS_NAME_
	.asciz	"Foo"

	.section	__DATA,__objc_const
	.p2align	3, 0x0                          ; @"_OBJC_METACLASS_RO_$_Foo"
__OBJC_METACLASS_RO_$_Foo:
	.long	3                               ; 0x3
	.long	40                              ; 0x28
	.long	40                              ; 0x28
	.space	4
	.quad	0
	.quad	l_OBJC_CLASS_NAME_
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0

	.section	__TEXT,__objc_methname,cstring_literals
l_OBJC_METH_VAR_NAME_:                  ; @OBJC_METH_VAR_NAME_
	.asciz	"m1"

	.section	__TEXT,__objc_methtype,cstring_literals
l_OBJC_METH_VAR_TYPE_:                  ; @OBJC_METH_VAR_TYPE_
	.asciz	"v16@0:8"

	.section	__TEXT,__objc_methname,cstring_literals
l_OBJC_METH_VAR_NAME_.1:                ; @OBJC_METH_VAR_NAME_.1
	.asciz	"m2"

l_OBJC_METH_VAR_NAME_.2:                ; @OBJC_METH_VAR_NAME_.2
	.asciz	"m3"

	.section	__DATA,__objc_const
	.p2align	3, 0x0                          ; @"_OBJC_$_INSTANCE_METHODS_Foo"
__OBJC_$_INSTANCE_METHODS_Foo:
	.long	24                              ; 0x18
	.long	3                               ; 0x3
	.quad	l_OBJC_METH_VAR_NAME_
	.quad	l_OBJC_METH_VAR_TYPE_
	.quad	"-[Foo m1]"
	.quad	l_OBJC_METH_VAR_NAME_.1
	.quad	l_OBJC_METH_VAR_TYPE_
	.quad	"-[Foo m2]"
	.quad	l_OBJC_METH_VAR_NAME_.2
	.quad	l_OBJC_METH_VAR_TYPE_
	.quad	"-[Foo m3]"

	.p2align	3, 0x0                          ; @"_OBJC_CLASS_RO_$_Foo"
__OBJC_CLASS_RO_$_Foo:
	.long	2                               ; 0x2
	.long	0                               ; 0x0
	.long	0                               ; 0x0
	.space	4
	.quad	0
	.quad	l_OBJC_CLASS_NAME_
	.quad	__OBJC_$_INSTANCE_METHODS_Foo
	.quad	0
	.quad	0
	.quad	0
	.quad	0

	.section	__DATA,__objc_selrefs,literal_pointers,no_dead_strip
	.p2align	3, 0x0                          ; @OBJC_SELECTOR_REFERENCES_
_OBJC_SELECTOR_REFERENCES_:
	.quad	l_OBJC_METH_VAR_NAME_

	.p2align	3, 0x0                          ; @OBJC_SELECTOR_REFERENCES_.3
_OBJC_SELECTOR_REFERENCES_.3:
	.quad	l_OBJC_METH_VAR_NAME_.1

	.p2align	3, 0x0                          ; @OBJC_SELECTOR_REFERENCES_.4
_OBJC_SELECTOR_REFERENCES_.4:
	.quad	l_OBJC_METH_VAR_NAME_.2

	.globl	__objc_empty_cache              ; @_objc_empty_cache
.zerofill __DATA,__common,__objc_empty_cache,8,3
	.section	__DATA,__objc_classlist,regular,no_dead_strip
	.p2align	3, 0x0                          ; @"OBJC_LABEL_CLASS_$"
l_OBJC_LABEL_CLASS_$:
	.quad	_OBJC_CLASS_$_Foo

	.section	__DATA,__objc_imageinfo,regular,no_dead_strip
L_OBJC_IMAGE_INFO:
	.long	0
	.long	64

.subsections_via_symbols
