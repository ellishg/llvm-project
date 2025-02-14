; RUN: opt < %s -mtriple=x86_64-unknown-linux -passes='function(memprof),memprof-module' -S | FileCheck %s --check-prefixes=EMPTY,COMDAT
; RUN: opt < %s -mtriple=arm64-apple-ios      -passes='function(memprof),memprof-module' -S | FileCheck %s --check-prefixes=EMPTY

; RUN: opt < %s -mtriple=x86_64-unknown-linux -passes='function(memprof),memprof-module' -S -memprof-runtime-default-options="verbose=1" | FileCheck %s --check-prefixes=VERBOSE,COMDAT
; RUN: opt < %s -mtriple=arm64-apple-ios      -passes='function(memprof),memprof-module' -S -memprof-runtime-default-options="verbose=1" | FileCheck %s --check-prefixes=VERBOSE

define i32 @main() {
entry:
  ret i32 0
}

; COMDAT: $__memprof_default_options_str = comdat any

; EMPTY: @__memprof_default_options_str = constant [1 x i8] zeroinitializer
; VERBOSE: @__memprof_default_options_str = constant [10 x i8] c"verbose=1\00"
