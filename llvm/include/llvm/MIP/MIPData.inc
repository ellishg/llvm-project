//===-- MIPData.inc - machine ir profile runtime structures -------*- C -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains definitions and data structures that are shared between
// the runtime and the compiler. It lives in two locations that need to stay in
// sync.
// + llvm/include/llvm/MIP/MIPData.inc
// + compiler-rt/include/mip/MIPData.inc
//
//===----------------------------------------------------------------------===//

#ifndef MIP_DATA_DEFINED

#include <stdint.h>

#ifndef MIP_DATA_INC
#define MIP_DATA_INC

#define MIP_SIMPLE_QUOTE(x) #x
#define MIP_QUOTE(x) MIP_SIMPLE_QUOTE(x)
#define MIP_SIMPLE_CONCAT(x,y) x ## y
#define MIP_CONCAT(x,y) MIP_SIMPLE_CONCAT(x,y)

#define MIP_RAW_SECTION __llvm_mipraw
#define MIP_MAP_SECTION __llvm_mipmap
#define MIP_RUNTIME_SYMBOL __llvm_mip_runtime

#define MIP_RAW_SECTION_NAME MIP_QUOTE(MIP_RAW_SECTION)
#define MIP_MAP_SECTION_NAME MIP_QUOTE(MIP_MAP_SECTION)
#define MIP_RUNTIME_SYMBOL_NAME MIP_QUOTE(MIP_RUNTIME_SYMBOL)

// MIP magic value in little endian format.
// \251 M    I    P
// 0xFB 0x4D 0x49 0x50
#define MIP_MAGIC_VALUE (0x50494DFB)
#define MIP_VERSION (8)

typedef enum {
  MIP_FILE_TYPE_RAW = 1 << 0,               // .mipraw
  MIP_FILE_TYPE_CALL_EDGE_SAMPLES = 1 << 1, // .mipret
  MIP_FILE_TYPE_MAP = 1 << 2,               // .mipmap
  MIP_FILE_TYPE_PROFILE = 1 << 3,           // .mip
  MIP_FILE_TYPE_64_BIT = 1 << 4,
  MIP_FILE_TYPE_32_BIT = 1 << 5,
} MIPFileType;

typedef enum {
  MIP_PROFILE_TYPE_FUNCTION_COVERAGE = 1 << 0,
  MIP_PROFILE_TYPE_BLOCK_COVERAGE = 1 << 1,
  MIP_PROFILE_TYPE_FUNCTION_TIMESTAMP = 1 << 2,
  MIP_PROFILE_TYPE_FUNCTION_CALL_COUNT = 1 << 3,
  MIP_PROFILE_TYPE_RETURN_ADDRESS = 1 << 4,
} MIPProfileType;

typedef struct {
  uint32_t Magic;
  uint16_t Version;
  uint16_t FileType;
  uint32_t ProfileType;
  uint32_t ModuleHash;
  int64_t RawSectionOffset;
  uint32_t Reserved;
  uint32_t OffsetToData;
} MIPHeader;

typedef struct {
  uint32_t CalleeProfileDataOffset;
  uint32_t SectionRelativeReturnAddress;
} CallEdge_t;

#endif // MIP_DATA_INC

#else // MIP_DATA_INC
#undef MIP_DATA_DEFINED
#endif // MIP_DATA_DEFINED
