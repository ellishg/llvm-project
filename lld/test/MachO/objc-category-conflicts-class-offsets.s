# REQUIRES: x86
# RUN: llvm-mc -filetype=obj -triple=x86_64-apple-macos11.0 %s -o %t.o
# RUN: %lld -no_objc_relative_method_lists --check-category-conflicts -dylib %t.o -o /dev/null
# RUN: %lld -no_objc_relative_method_lists -objc_category_merging -dylib %t.o -o /dev/null
# RUN: llvm-mc -filetype=obj -triple=x86_64-apple-macos11.0 --defsym CLASS_CONFLICT=1 %s -o %t.conflict.o
# RUN: %no-fatal-warnings-lld -no_objc_relative_method_lists --check-category-conflicts -dylib %t.conflict.o -o /dev/null 2>&1 | \
# RUN:   FileCheck %s --implicit-check-not "method '-shared'"
# RUN: %no-fatal-warnings-lld -no_objc_relative_method_lists -objc_category_merging -dylib %t.conflict.o -o /dev/null 2>&1 | \
# RUN:   FileCheck %s --implicit-check-not "method '-shared'"

# RUN: llvm-mc -filetype=obj -triple=x86_64-apple-macos11.0 --defsym CLASS_CONFLICT=1 --defsym PACKED_RODATA=1 %s -o %t.packed.conflict.o
# RUN: %no-fatal-warnings-lld -no_objc_relative_method_lists --check-category-conflicts -dylib %t.packed.conflict.o -o /dev/null 2>&1 | \
# RUN:   FileCheck %s --implicit-check-not "method '-shared'"
# RUN: %no-fatal-warnings-lld -no_objc_relative_method_lists -objc_category_merging -dylib %t.packed.conflict.o -o %t.packed.dylib 2>&1 | \
# RUN:   FileCheck %s --implicit-check-not "method '-shared'" --implicit-check-not 'merging skipped'
# RUN: llvm-objdump --macho --objc-meta-data %t.packed.dylib | FileCheck %s --check-prefix=PACKED

# RUN: llvm-mc -filetype=obj -triple=x86_64-apple-macos11.0 --defsym CLASS_CONFLICT=1 --defsym RO_FLAGS=2 %s -o %t.tagged.conflict.o
# RUN: %no-fatal-warnings-lld -no_objc_relative_method_lists --check-category-conflicts -dylib %t.tagged.conflict.o -o /dev/null 2>&1 | \
# RUN:   FileCheck %s --implicit-check-not "method '-shared'"
# RUN: %no-fatal-warnings-lld -no_objc_relative_method_lists -objc_category_merging -dylib %t.tagged.conflict.o -o /dev/null 2>&1 | \
# RUN:   FileCheck %s --implicit-check-not "method '-shared'" --implicit-check-not 'merging skipped'

## Two class address points can share a containing Swift metadata symbol.
## Categories for Foo and Bar may define the same selector without conflicting.
## Bar's own methods must still be parsed, even though Foo was seen first.
# CHECK: warning: method '-barOnly' has conflicting definitions:
# CHECK-NEXT: >>> defined in category CatBar from {{.*}}.conflict.o
# CHECK-NEXT: >>> defined in class Bar from {{.*}}.conflict.o
# PACKED: name {{.*}} Foo
# PACKED: baseMethods
# PACKED-NEXT: entsize 24
# PACKED-NEXT: count 2
# PACKED: name {{.*}} shared
# PACKED: name {{.*}} fooOnly
# PACKED: name {{.*}} Bar
# PACKED: baseMethods
# PACKED-NEXT: entsize 24
# PACKED-NEXT: count 3
# PACKED: name {{.*}} barOnly
# PACKED: name {{.*}} shared
# PACKED: name {{.*}} barOnly

.text
method:
  ret

.section __DATA,__objc_data
.globl _Metadata
_Metadata:
.ifndef RO_FLAGS
.set RO_FLAGS, 0
.endif
  # Foo and its metaclass.
  .quad _Metadata+40, 0, 0, 0, FooRO+RO_FLAGS
  .quad 0, 0, 0, 0, FooMetaRO+RO_FLAGS
  # Bar and its metaclass. There are no symbols at these address points.
.ifdef PACKED_RODATA
  .quad _Metadata+120, 0, 0, 0, FooRO+72+RO_FLAGS
  .quad 0, 0, 0, 0, FooMetaRO+72+RO_FLAGS
.else
  .quad _Metadata+120, 0, 0, 0, BarRO+RO_FLAGS
  .quad 0, 0, 0, 0, BarMetaRO+RO_FLAGS
.endif

.section __TEXT,__objc_classname,cstring_literals
fooName:
  .asciz "Foo"
barName:
  .asciz "Bar"
catFooName:
  .asciz "CatFoo"
catBarName:
  .asciz "CatBar"

.section __TEXT,__objc_methname,cstring_literals
shared:
  .asciz "shared"
fooOnly:
  .asciz "fooOnly"
barOnly:
  .asciz "barOnly"
.section __TEXT,__objc_methtype,cstring_literals
types:
  .asciz "v16@0:8"

.section __DATA,__objc_const
.macro rodata name, methods
  .long 0, 0, 0, 0
  .quad 0, \name, \methods, 0, 0, 0, 0
.endm
FooRO:
  rodata fooName, FooMethods
.ifndef PACKED_RODATA
BarRO:
.endif
  rodata barName, BarMethods
FooMetaRO:
  rodata fooName, 0
.ifndef PACKED_RODATA
BarMetaRO:
.endif
  rodata barName, 0

FooMethods:
  .long 24, 1
  .quad fooOnly, types, method
BarMethods:
  .long 24, 1
  .quad barOnly, types, method

catFoo:
  .quad catFooName, _Metadata, CatFooMethods, 0, 0, 0, 0
  .long 64, 0
catBar:
  .quad catBarName, _Metadata+80, CatBarMethods, 0, 0, 0, 0
  .long 64, 0
CatFooMethods:
  .long 24, 1
  .quad shared, types, method
CatBarMethods:
  .long 24
.ifdef CLASS_CONFLICT
  .long 2
  .quad barOnly, types, method
.else
  .long 1
.endif
  .quad shared, types, method

.section __DATA,__objc_classlist,regular,no_dead_strip
  .quad _Metadata, _Metadata+80

.section __DATA,__objc_catlist,regular,no_dead_strip
  .quad catFoo
.section __DATA,__objc_catlist,regular,no_dead_strip
catBarList:
  .quad catBar

.subsections_via_symbols
