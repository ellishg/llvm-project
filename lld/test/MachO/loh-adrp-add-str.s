# REQUIRES: aarch64

# RUN: llvm-mc -filetype=obj -triple=arm64-apple-darwin %s -o %t.o
# RUN: %lld -arch arm64 %t.o -o %t
# RUN: llvm-objdump --no-print-imm-hex -d --macho %t | FileCheck %s

.text
.align 2
.globl _main
_main:
# CHECK-LABEL: _main:

### Transformation to ADRP + immediate STR
## Basic test: target is far
L28: adrp x0, _far@PAGE
L29: add  x1, x0, _far@PAGEOFF
L30: str  w2, [x1]
# CHECK-NEXT: adrp x0,
# CHECK-NEXT: nop
# CHECK-NEXT: str w2, [x0,

L31: adrp x0, _far@PAGE
L32: add  x1, x0, _far@PAGEOFF
L33: str  x2, [x1]
# CHECK-NEXT: adrp x0,
# CHECK-NEXT: nop
# CHECK-NEXT: str x2, [x0,

## With offset
L34: adrp x0, _far@PAGE
L35: add  x1, x0, _far@PAGEOFF
L36: str  x2, [x1, #8]
# CHECK-NEXT: adrp x0,
# CHECK-NEXT: nop
# CHECK-NEXT: str x2, [x0,

L100: adrp x0, _far@PAGE
L101: add  x1, x0, _far@PAGEOFF
L102: stp  x3, x2, [x1]
# CHECK-NEXT: adrp x0,
# CHECK-NEXT: nop
# CHECK-NEXT: stp x3, x2, [x0,

L110: adrp x0, _far@PAGE
L111: add  x1, x0, _far@PAGEOFF
L112: stp  x3, x2, [x1, #16]
# CHECK-NEXT: adrp x0,
# CHECK-NEXT: nop
# CHECK-NEXT: stp x3, x2, [x0,

L120: adrp x0, _far@PAGE
L121: add  x1, x0, _far@PAGEOFF
L122: stp  w3, w2, [x1, #-32]
# CHECK-NEXT: adrp x0,
# CHECK-NEXT: nop
# CHECK-NEXT: stp w3, w2, [x0,

### No changes
## Far and unaligned
L37: adrp x0, _far_unaligned@PAGE
L38: add  x1, x0, _far_unaligned@PAGEOFF
L39: str  x2, [x1]
# CHECK-NEXT: adrp x0,
# CHECK-NEXT: add x1, x0
# CHECK-NEXT: str x2, [x1]

## Far with large offset (_far_offset@PAGE + #255 > 4095)
L40: adrp x0, _far_offset@PAGE
L41: add  x1, x0, _far_offset@PAGEOFF
L42: strb w2, [x1, #255]
# CHECK-NEXT: adrp x0,
# CHECK-NEXT: add x1, x0
# CHECK-NEXT: strb w2, [x1, #255]

L130: adrp x0, _far@PAGE
L131: add  x1, x0, _far@PAGEOFF
L132: stp  x3, x2, [x1, #504]
# CHECK-NEXT: adrp x0,
# CHECK-NEXT: add x1, x0, #8
# CHECK-NEXT: stp x3, x2, [x1, #504]

L140: adrp x0, _far@PAGE
L141: add  x1, x0, _far@PAGEOFF
L142: stp  w3, w2, [x1, #252]
# CHECK-NEXT: adrp x0,
# CHECK-NEXT: add x1, x0, #8
# CHECK-NEXT: stp w3, w2, [x1, #252]

## Registers don't match
L43: adrp x0, _far@PAGE
L44: add  x1, x0, _far@PAGEOFF
L45: str  x2, [x2]
# CHECK-NEXT: adrp x0
# CHECK-NEXT: add x1, x0
# CHECK-NEXT: str x2, [x2]

.data
.align 4
    .quad 0
_unaligned:
    .quad 0

.space 1048576
.align 12
    .quad 0
_far:
     .quad 0
    .byte 0
_far_unaligned:
    .quad 0
.space 4000
_far_offset:
    .byte 0

.loh AdrpAddStr L28, L29, L30
.loh AdrpAddStr L31, L32, L33
.loh AdrpAddStr L34, L35, L36
.loh AdrpAddStr L37, L38, L39
.loh AdrpAddStr L40, L41, L42
.loh AdrpAddStr L100, L101, L102
.loh AdrpAddStr L110, L111, L112
.loh AdrpAddStr L120, L121, L122
.loh AdrpAddStr L130, L131, L132
.loh AdrpAddStr L140, L141, L142
