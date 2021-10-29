//===- MIP.h - Machine IR Profile -------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_MIP_MIP_H
#define LLVM_MIP_MIP_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/DebugInfo/DIContext.h"
#include <string>
#include <vector>

namespace llvm {
namespace MachineProfile {

#include "llvm/MIP/MIPData.inc"

// Profile of a function call to a particular machine function
struct CallEdge {
  // The section-relative address of the callsite
  uint32_t SectionRelativeSourceAddress;
  // The weight associated with this call edge
  uint32_t Weight;
};

// Machine IR profile data of a machine basic block.
struct MBBProfile {
  // Function-relative machine basic block offset
  uint32_t Offset;
  // True if this block was executed
  bool IsCovered;

  MBBProfile() : Offset(0), IsCovered(false) {}
  MBBProfile(uint32_t Offset) : Offset(Offset), IsCovered(false) {}
};

// Machine IR profile data of a machine function.
struct MFProfile {
  std::string FunctionName;
  // MD5 hash of the function name
  uint64_t FunctionSignature;
  // Section-relative raw profile address
  uint64_t RawProfileDataAddress;
  // Function address offset to raw section
  int64_t EncodedFunctionAddress;
  // Function size
  uint32_t FunctionSize;
  // MD5 hash of the control flow graph of the function
  uint32_t ControlFlowGraphSignature;
  // The number of raw profiles accumulated into this profile
  uint32_t RawProfileCount;
  // The number of times this function was called.
  uint64_t FunctionCallCount;
  // Accumulation over all raw profiles of the order index of this function
  uint64_t FunctionOrderSum;
  // Profiles of machine basic blocks
  SmallVector<MBBProfile, 8> BasicBlockProfiles;
  // Profiles of incoming machine function calls
  SmallVector<CallEdge, 8> CallEdges;
  // Source info about this function if available
  Optional<DILineInfo> SourceInfo;
};

// Machine IR profile data of a particular module.
struct MIRProfile {
public:
  MIPHeader Header;
  std::vector<MFProfile> Profiles;

  void getOrderedProfiles(std::vector<MFProfile> &OrderedProfiles) const;
};

// Machine IR raw profile data of a machine function.
struct MFRawProfile {
  // Section-relative raw profile address
  uint64_t RawProfileDataAddress = 0;

  // MIPProfileType::FUNCTION_COVERAGE
  bool IsFunctionCovered = 0;

  // MIPProfileType::BLOCK_COVERAGE
  SmallVector<bool, 8> BasicBlockCoverage;

  // MIPProfileType::FUNCTION_CALL_COUNT
  uint32_t FunctionCallCount = 0;

  // MIPProfileType::FUNCTION_TIMESTAMP
  uint32_t FunctionTimestamp = 0;
};

// Machine IR raw profile data of a module.
struct MIRRawProfile {
  MIPHeader Header;
  std::vector<MFRawProfile> RawProfiles;
  std::vector<CallEdge_t> RawCallEdges;
};

} // namespace MachineProfile
} // namespace llvm

#endif // LLVM_MIP_MIP_H
