// Test if memprof instrumentation and use pass are invoked.
//
// RUN: rm -rf %t && split-file %s %t

// Instrumentation:
// Ensure Pass MemProfilerPass and ModuleMemProfilerPass are invoked.
// RUN: %clang_cc1 -O2 -fmemory-profile %t/memprof.cpp -fdebug-pass-manager -emit-llvm -o - 2>&1 | FileCheck %s -check-prefix=INSTRUMENT
// INSTRUMENT: Running pass: MemProfilerPass on main
// INSTRUMENT: Running pass: ModuleMemProfilerPass on [module]

// RUN: llvm-profdata merge %t/memprof.yaml -o %t.memprofdata

// Profile use:
// Ensure Pass PGOInstrumentationUse is invoked with the memprof-only profile.
// RUN: %clang_cc1 -O2 -fmemory-profile-use=%t.memprofdata %t/memprof.cpp -fdebug-pass-manager -emit-llvm -o - 2>&1 | FileCheck %s -check-prefix=USE
// USE: Running pass: MemProfUsePass on [module]

//--- memprof.cpp
char *foo() {
  return new char[10];
}
int main() {
  char *a = foo();
  delete[] a;
  return 0;
}

//--- memprof.yaml
---
HeapProfileRecords:
  - GUID:            0x1
    AllocSites:
      - Callstack:
          - { Function: 0x1, LineOffset: 0, Column: 0, IsInlineFrame: false }
        MemInfoBlock:
          AllocCount:      1
          TotalSize:       1
          TotalLifetime:   0
          TotalLifetimeAccessDensity: 0
...
