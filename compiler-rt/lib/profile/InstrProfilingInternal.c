/*===- InstrProfilingInternal.c - Support library for PGO instrumentation -===*\
|*
|* Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
|* See https://llvm.org/LICENSE.txt for license information.
|* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
|*
\*===----------------------------------------------------------------------===*/

// Note: This is linked into the Darwin kernel, and must remain compatible
// with freestanding compilation. See `darwin_add_builtin_libraries`.

#if defined(__ELF__)
#include <elf.h>
#include <link.h>
#elif defined(__APPLE__) && !defined(KERNEL_USE)
#include <dlfcn.h>
#include <mach-o/dyld.h>
#endif

#include "InstrProfilingInternal.h"

// TODO: Implement in InstrProfilingPlatformDarwin.c
COMPILER_RT_VISIBILITY uintptr_t lprofGetLoadBias(void) {
#if defined(__ELF__) && defined(NT_GNU_BUILD_ID)
  extern const ElfW(Ehdr) __ehdr_start __attribute__((visibility("hidden")));
  extern ElfW(Dyn) _DYNAMIC[] __attribute__((weak, visibility("hidden")));

  const ElfW(Ehdr) *ElfHeader = &__ehdr_start;
  const ElfW(Phdr) *ProgramHeader =
      (const ElfW(Phdr) *)((uintptr_t)ElfHeader + ElfHeader->e_phoff);
  uintptr_t Base = 0;
  for (uint32_t I = 0; I < ElfHeader->e_phnum; I++) {
    if (ProgramHeader[I].p_type == PT_PHDR)
      Base = (uintptr_t)ProgramHeader - ProgramHeader[I].p_vaddr;
    if (ProgramHeader[I].p_type == PT_DYNAMIC && _DYNAMIC)
      Base = (uintptr_t)_DYNAMIC - ProgramHeader[I].p_vaddr;
  }
  return Base;
#elif defined(__APPLE__) && !defined(KERNEL_USE)
  Dl_info Info;
  if (!dladdr((const void *)&lprofGetLoadBias, &Info) || !Info.dli_fbase)
    return 0;

  uint32_t NumImages = _dyld_image_count();
  for (uint32_t I = 0; I < NumImages; ++I)
    if ((const void *)_dyld_get_image_header(I) == Info.dli_fbase)
      return (uintptr_t)_dyld_get_image_vmaddr_slide(I);
  return 0;
#else
  return 0;
#endif
}

#if !defined(__Fuchsia__)

static unsigned ProfileDumped = 0;

COMPILER_RT_VISIBILITY unsigned lprofProfileDumped(void) {
  return ProfileDumped;
}

COMPILER_RT_VISIBILITY void lprofSetProfileDumped(unsigned Value) {
  ProfileDumped = Value;
}

#endif
