/*===- MIPHelper.c - Machine IR Profile Runtime Helper --------------------===*\
|*
|* Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
|* See https://llvm.org/LICENSE.txt for license information.
|* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
|*
\*===----------------------------------------------------------------------===*/

#include "MIPHelper.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void __llvm_mip_runtime_initialize(void) { atexit(__llvm_dump_mip_profile); }

void __llvm_dump_mip_profile(void) {
  const char *Filename = getenv("LLVM_MIP_PROFILE_FILENAME");
  if (!Filename || !Filename[0])
    Filename = "default.mipraw";

  char *FormatSpecifierPtr;
  char Buffer[strlen(Filename) + 3];
  if ((FormatSpecifierPtr = strstr(Filename, "%h"))) {
    const MIPHeader *Header = (MIPHeader *)__llvm_mip_profile_begin();
    const char *Prefix = Filename;
    int PrefixLength = FormatSpecifierPtr - Filename;
    const char *Suffix = FormatSpecifierPtr + strlen("%h");
    sprintf(Buffer, "%.*s%04x%s", PrefixLength, Prefix,
            Header->ModuleHash & 0xFFFF, Suffix);
    Filename = Buffer;
  }
  __llvm_dump_mip_profile_with_filename(Filename);
}

int __llvm_dump_mip_profile_with_filename(const char *Filename) {
  FILE *filep = fopen(Filename, "wb");
  if (!filep) {
    fprintf(stderr, "[MIPRuntime]: Failed to open %s: %s\n", Filename,
            strerror(errno));
    return -1;
  }

  const void *Data = __llvm_mip_profile_begin();
  size_t DataSize =
      (char *)__llvm_mip_profile_end() - (char *)__llvm_mip_profile_begin();
  size_t BytesWritten = fwrite(Data, 1, DataSize, filep);
  if (BytesWritten != DataSize) {
    fprintf(stderr, "[MIPRuntime]: Failed to write to %s: %s\n", Filename,
            strerror(errno));
    fclose(filep);
    return -1;
  }

  fclose(filep);
  return 0;
}

#ifdef __linux__
#define MIP_RAW_SECTION_BEGIN_SYMBOL MIP_CONCAT(__start_, MIP_RAW_SECTION)
#define MIP_RAW_SECTION_END_SYMBOL MIP_CONCAT(__stop_, MIP_RAW_SECTION)
extern char MIP_RAW_SECTION_BEGIN_SYMBOL;
extern char MIP_RAW_SECTION_END_SYMBOL;
#endif // __linux__

#ifdef __APPLE__
#define MIP_RAW_SECTION_BEGIN_SYMBOL __llvm_mip_raw_section_start
#define MIP_RAW_SECTION_END_SYMBOL __llvm_mip_raw_section_end
extern char MIP_RAW_SECTION_BEGIN_SYMBOL __asm(
    "section$start$__DATA$" MIP_RAW_SECTION_NAME);
extern char MIP_RAW_SECTION_END_SYMBOL __asm(
    "section$end$__DATA$" MIP_RAW_SECTION_NAME);
#endif // __APPLE__

void *__llvm_mip_profile_begin(void) { return &MIP_RAW_SECTION_BEGIN_SYMBOL; }

void *__llvm_mip_profile_end(void) { return &MIP_RAW_SECTION_END_SYMBOL; }
