; RUN: llc < %s -enable-machine-instrumentation -enable-machine-call-graph -mtriple=x86_64-linux | FileCheck %s --check-prefix ELF
; RUN: llc < %s -enable-machine-instrumentation -enable-machine-call-graph -mtriple=x86_64-apple-macosx | FileCheck %s --check-prefix MACHO

define i32 @_Z3fooii(i32 %a, i32 %b) #0 {
  ret i32 0
}

;================================= .mipraw Header =====================================;
; ELF-LABEL:   .section  __llvm_mipraw,"aGwR",@progbits,__llvm_mipraw_header,comdat
; ELF:         .p2align  3
; ELF:         .long    0x50494dfb                      # Magic
; ELF-NEXT:    .short   8                               # Version
; ELF-NEXT:    .short   0x11                            # File Type
; ELF-NEXT:    .long    0xc                             # Profile Type
; ELF-NEXT:    .long    [[MODULE_HASH:.*]]              # Module Hash
; ELF-NEXT:    .zero    8
; ELF-NEXT:    .zero    4
; ELF-NEXT:    .long    0x20                            # Offset To Data

;================================= .mipmap Header =====================================;
; ELF-LABEL:   .section  __llvm_mipmap,"GwR",@progbits,__llvm_mipmap_header,comdat
; ELF:         .p2align  3
; ELF-NEXT:  [[REF:.Lref.+]]:
; ELF-NEXT:    .long    0x50494dfb                      # Magic
; ELF-NEXT:    .short   8                               # Version
; ELF-NEXT:    .short   0x14                            # File Type
; ELF-NEXT:    .long    0xc                             # Profile Type
; ELF-NEXT:    .long    [[MODULE_HASH]]                 # Module Hash
; ELF-NEXT:    .quad    __start___llvm_mipraw-[[REF]]   # Raw Section Start PC Offset
; ELF-NEXT:    .zero    4
; ELF-NEXT:    .long    0x20                            # Offset To Data

;================================= .mipraw Header =====================================;
; MACHO:       .section __DATA,__llvm_mipraw
; MACHO:       .globl   __header$__llvm_mipraw
; MACHO:       .weak_definition  __header$__llvm_mipraw
; MACHO:       .no_dead_strip __header$__llvm_mipraw
; MACHO-LABEL: __header$__llvm_mipraw:
; MACHO:       .p2align 3
; MACHO:       .long   0x50494dfb              ## Magic
; MACHO-NEXT:  .short  8                       ## Version
; MACHO-NEXT:  .short  0x11                    ## File Type
; MACHO-NEXT:  .long   0xc                     ## Profile Type
; MACHO-NEXT:  .long   [[MODULE_HASH:.*]]      ## Module Hash
; MACHO-NEXT:  .space  8
; MACHO-NEXT:  .space  4
; MACHO-NEXT:  .long   0x20                    ## Offset To Data


;================================= .mipmap Header =====================================;
; MACHO:       .section __DATA,__llvm_mipmap,regular,live_support
; MACHO:       .globl   __header$__llvm_mipmap
; MACHO:       .weak_definition  __header$__llvm_mipmap
; MACHO-LABEL: __header$__llvm_mipmap:
; MACHO:       .p2align 3
; MACHO:     [[REF:Lref.+]]:
; MACHO:       .long   0x50494dfb              ## Magic
; MACHO-NEXT:  .short  8                       ## Version
; MACHO-NEXT:  .short  0x14                    ## File Type
; MACHO-NEXT:  .long   0xc                     ## Profile Type
; MACHO-NEXT:  .long   [[MODULE_HASH]]         ## Module Hash
; MACHO-NEXT:  .quad    __header$__llvm_mipraw-[[REF]] ## Raw Section Start PC Offset
; MACHO-NEXT:  .space  4
; MACHO-NEXT:  .long   0x20                    ## Offset To Data
