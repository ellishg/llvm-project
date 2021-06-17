//===- MIPSectionEmitter.h --------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_MIPSECTIONEMITTER_H
#define LLVM_CODEGEN_MIPSECTIONEMITTER_H

#include "MachineFunction.h"
#include "MachineOperand.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MIP/MIP.h"
#include "llvm/Support/Endian.h"
#include <map>

namespace llvm {

class AsmPrinter;

class MIPSectionEmitter {
public:
  explicit MIPSectionEmitter(AsmPrinter &AP);

  void runOnMachineFunctionStart(MachineFunction &MF);
  void runOnMachineFunctionEnd(MachineFunction &MF);
  void runOnFunctionInstrumentationMarker(const MachineInstr &MI);
  void runOnBasicBlockInstrumentationMarker(const MachineInstr &MI);
  void serializeToMIPRawSection();
  void serializeToMIPMapSection();

  MCSymbol *getRawProfileSymbol(const MachineFunction &MF);
  uint64_t getOffsetToRawBlockProfileSymbol(uint32_t BlockID);

private:
  struct MBBInfo {
    const MCSymbol *StartSymbol;
  };

  struct MFInfo {
    const Function *Func;
    const MCSymbol *StartSymbol;
    const MCSymbol *EndSymbol;
    MCSymbol *RawProfileSymbol;
    uint32_t ControlFlowGraphSignature;
    uint32_t NonEntryBasicBlockCount;

    // A map from Machine Basic Block IDs to MBBInfo.
    DenseMap<uint32_t, MBBInfo> BasicBlockInfos;
  };

  void emitMIPHeader(MachineProfile::MIPFileType FileType);
  void emitMIPFunctionData(const MFInfo &Info);
  void emitMIPFunctionInfo(MFInfo &Info);
  MCSymbol *getMIPSectionBeginSymbol(Twine MIPSectionName);
  void emitLinkageAndVisibility(MCSymbol *Sym);

  AsmPrinter &AP;
  MCSymbol *CurrentFunctionEndSymbol;

  // A map from a function symbol to its function info.
  std::map<const MCSymbol *, MFInfo> FunctionInfos;
};

} // end namespace llvm

#endif // LLVM_CODEGEN_MIPSECTIONEMITTER_H
