/*===- MIPHelper.h - Machine IR Profile Runtime Helper --------------------===*\
|*
|* Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
|* See https://llvm.org/LICENSE.txt for license information.
|* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
|*
\*===----------------------------------------------------------------------===*/

#ifndef MIP_MIPHELPER_H
#define MIP_MIPHELPER_H

#include "mip/MIPData.inc"

void __llvm_dump_mip_profile(void);
int __llvm_dump_mip_profile_with_filename(const char *Filename);

void *__llvm_mip_profile_begin(void);
void *__llvm_mip_profile_end(void);

void __llvm_mip_runtime_initialize(void);

typedef struct {
  uint32_t CallCount;
  uint32_t Timestamp;
  // uint8_t BlockCoverage[BlockCount];
} ProfileData_t;

#endif // MIP_MIPHELPER_H
