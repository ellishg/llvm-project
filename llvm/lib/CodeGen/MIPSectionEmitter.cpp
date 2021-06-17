//===- MIPSectionEmitter.cpp ----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/MIPSectionEmitter.h"
#include "llvm/ADT/Twine.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MIRInstrumentationPass.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Mangler.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCObjectStreamer.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;
using namespace llvm::MachineProfile;

std::string getMangledName(const Function *F) {
  std::string MangledName;
  raw_string_ostream MangledNameOS(MangledName);
  Mangler().getNameWithPrefix(MangledNameOS, F,
                              /*CannotUsePrivateLabel=*/true);
  return MangledName;
}

void MIPSectionEmitter::emitLinkageAndVisibility(MCSymbol *Sym) {
  auto &OS = *AP.OutStreamer;
  OS.emitSymbolAttribute(Sym, AP.MAI->getHiddenVisibilityAttr());
  OS.emitSymbolAttribute(Sym, MCSymbolAttr::MCSA_Weak);
}

MIPSectionEmitter::MIPSectionEmitter(AsmPrinter &AP) : AP(AP) {}

MCSymbol *MIPSectionEmitter::getMIPSectionBeginSymbol(Twine MIPSectionName) {
  auto &OS = *AP.OutStreamer;
  auto &OutContext = OS.getContext();
  const auto &TT = OutContext.getTargetTriple();

  if (TT.getObjectFormat() == Triple::ELF)
    return OutContext.getOrCreateSymbol("__start_" + MIPSectionName);
  if (TT.getObjectFormat() == Triple::MachO)
    return OutContext.getOrCreateSymbol("__header$" + MIPSectionName);
  llvm_unreachable("Unsupported target triple");
}

void MIPSectionEmitter::runOnMachineFunctionStart(MachineFunction &MF) {
  if (!MIRInstrumentation::EnableMachineInstrumentation)
    return;

  auto &OS = *AP.OutStreamer;
  auto &OutContext = OS.getContext();

  for (auto &MBB : MF)
    MBB.setLabelMustBeEmitted();
  CurrentFunctionEndSymbol = OutContext.createTempSymbol("mip_func_end");
}

void MIPSectionEmitter::runOnMachineFunctionEnd(MachineFunction &MF) {
  if (!MIRInstrumentation::EnableMachineInstrumentation)
    return;

  auto &OS = *AP.OutStreamer;
  OS.emitLabel(CurrentFunctionEndSymbol);
}

void MIPSectionEmitter::runOnFunctionInstrumentationMarker(
    const MachineInstr &MI) {
  assert(MI.getOpcode() == TargetOpcode::MIP_FUNCTION_INSTRUMENTATION_MARKER);
  auto &OS = *AP.OutStreamer;
  auto &OutContext = OS.getContext();

  MFInfo Info;
  Info.Func = &MI.getMF()->getFunction();
  Info.StartSymbol = AP.TM.getSymbol(Info.Func);
  Info.EndSymbol = CurrentFunctionEndSymbol;
  Info.RawProfileSymbol =
      OutContext.getOrCreateSymbol(getMangledName(Info.Func) + "$RAW");
  Info.ControlFlowGraphSignature = MI.getOperand(0).getImm();
  Info.NonEntryBasicBlockCount = MI.getOperand(1).getImm();

  FunctionInfos.insert(std::make_pair(Info.StartSymbol, Info));
}

void MIPSectionEmitter::runOnBasicBlockInstrumentationMarker(
    const MachineInstr &MI) {
  assert(MI.getOpcode() ==
         TargetOpcode::MIP_BASIC_BLOCK_COVERAGE_INSTRUMENTATION);
  // TODO: Properly lookup the correct function info instead of just looking at
  //       the function we are currently in.
  const auto &F = MI.getMF()->getFunction();
  auto BlockID = MI.getOperand(1).getImm();
  MBBInfo Info;
  Info.StartSymbol = MI.getParent()->getSymbol();
  auto *FunctionSymbol = AP.TM.getSymbol(&F);
  auto &FunctionInfo = FunctionInfos[FunctionSymbol];
  FunctionInfo.BasicBlockInfos.insert(std::make_pair(BlockID, Info));
}

MCSymbol *MIPSectionEmitter::getRawProfileSymbol(const MachineFunction &MF) {
  // TODO: Take a function symbol as input so we know we are getting the right
  // raw symbol. For now we are using the function we are currently in.
  auto *FunctionSymbol = AP.TM.getSymbol(&MF.getFunction());
  auto &Info = FunctionInfos[FunctionSymbol];
  return Info.RawProfileSymbol;
}

uint64_t MIPSectionEmitter::getOffsetToRawBlockProfileSymbol(uint32_t BlockID) {
  assert(MIRInstrumentation::EnableMachineBasicBlockCoverage);
  if (MIRInstrumentation::EnableMachineFunctionCoverage)
    return 1 + BlockID;
  if (MIRInstrumentation::EnableMachineCallGraph)
    return 8 + BlockID;
  llvm_unreachable("Expected function coverage or call graph instrumentation.");
}

void MIPSectionEmitter::emitMIPHeader(MIPFileType FileType) {
  auto &OS = *AP.OutStreamer;
  auto &OutContext = OS.getContext();
  const auto &TT = OutContext.getTargetTriple();
  if (TT.isArch64Bit()) {
    FileType = (MIPFileType)(FileType | MIP_FILE_TYPE_64_BIT);
  } else if (TT.isArch32Bit()) {
    FileType = (MIPFileType)(FileType | MIP_FILE_TYPE_32_BIT);
  } else {
    llvm_unreachable("Only 64 bit and 32 bit architectures are supported.");
  }

  OS.emitValueToAlignment(TT.isArch64Bit() ? 8 : 4);

  auto *ReferenceLabel = OutContext.createTempSymbol("ref");
  OS.emitLabel(ReferenceLabel);

  OS.AddComment("Magic");
  OS.emitIntValueInHex(MIP_MAGIC_VALUE, 4);

  OS.AddComment("Version");
  OS.emitIntValue(MIP_VERSION, 2);

  OS.AddComment("File Type");
  OS.emitIntValueInHex(FileType, 2);

  uint32_t ProfileType;
  if (MIRInstrumentation::EnableMachineFunctionCoverage) {
    ProfileType = MIP_PROFILE_TYPE_FUNCTION_COVERAGE;
  } else if (MIRInstrumentation::EnableMachineCallGraph) {
    ProfileType = MIP_PROFILE_TYPE_FUNCTION_TIMESTAMP |
                  MIP_PROFILE_TYPE_FUNCTION_CALL_COUNT;
  } else {
    llvm_unreachable(
        "Expected function coverage or call graph instrumentation.");
  }
  if (MIRInstrumentation::EnableMachineBasicBlockCoverage)
    ProfileType |= MIP_PROFILE_TYPE_BLOCK_COVERAGE;
  OS.AddComment("Profile Type");
  OS.emitIntValueInHex(ProfileType, 4);

  OS.AddComment("Module Hash");
  OS.emitIntValueInHex((uint32_t)MD5Hash(MIRInstrumentation::LinkUnitName), 4);

  if (FileType & MIP_FILE_TYPE_MAP) {
    OS.AddComment("Raw Section Start PC Offset");
    OS.emitValue(
        MCBinaryExpr::createSub(
            MCSymbolRefExpr::create(
                getMIPSectionBeginSymbol(MIP_RAW_SECTION_NAME), OutContext),
            MCSymbolRefExpr::create(ReferenceLabel, OutContext), OutContext),
        TT.isArch64Bit() ? 8 : 4);
  } else {
    OS.emitZeros(TT.isArch64Bit() ? 8 : 4);
  }
  if (TT.isArch32Bit())
    OS.emitZeros(4);
  OS.emitZeros(4);

  OS.AddComment("Offset To Data");
  OS.emitIntValueInHex(sizeof(MIPHeader), 4);

  OS.AddBlankLine();
}

void MIPSectionEmitter::emitMIPFunctionData(const MFInfo &Info) {
  auto &OS = *AP.OutStreamer;
  auto &OutContext = OS.getContext();
  const auto &TT = OutContext.getTargetTriple();
  auto MangledName = getMangledName(Info.Func);

  MCSection *RawSection;
  if (TT.getObjectFormat() == Triple::ELF) {
    if (Info.Func->hasComdat()) {
      auto ComdatName = Info.Func->getComdat()->getName();
      RawSection = (MCSection *)OutContext.getELFSection(
          MIP_RAW_SECTION_NAME, ELF::SHT_PROGBITS,
          ELF::SHF_WRITE | ELF::SHF_ALLOC | ELF::SHF_GROUP, 0, ComdatName,
          /*IsComdat=*/true);
    } else {
      auto GroupName = MangledName + "$MIP";
      RawSection = (MCSection *)OutContext.getELFSection(
          MIP_RAW_SECTION_NAME, ELF::SHT_PROGBITS,
          ELF::SHF_WRITE | ELF::SHF_ALLOC | ELF::SHF_GROUP, 0, GroupName,
          /*IsComdat=*/false);
    }
  } else {
    RawSection = OutContext.getObjectFileInfo()->getMIPRawSection();
  }
  OS.SwitchSection(RawSection);

  emitLinkageAndVisibility(Info.RawProfileSymbol);
  if (MIRInstrumentation::EnableMachineFunctionCoverage) {
    OS.emitValueToAlignment(1);
    OS.emitLabel(Info.RawProfileSymbol);

    OS.emitIntValueInHex(0xFF, 1);
  } else if (MIRInstrumentation::EnableMachineCallGraph) {
    OS.emitValueToAlignment(4);
    OS.emitLabel(Info.RawProfileSymbol);

    OS.emitIntValueInHex(0xFFFFFFFF, 4);
    OS.emitIntValueInHex(0xFFFFFFFF, 4);
  } else {
    llvm_unreachable(
        "Expected function coverage or call graph instrumentation.");
  }

  if (MIRInstrumentation::EnableMachineBasicBlockCoverage) {
    OS.emitFill(Info.NonEntryBasicBlockCount, 0xFF);
  }

  OS.AddBlankLine();
}

void MIPSectionEmitter::emitMIPFunctionInfo(MFInfo &Info) {
  auto &OS = *AP.OutStreamer;
  auto &OutContext = OS.getContext();
  const auto &TT = OutContext.getTargetTriple();
  auto MangledName = getMangledName(Info.Func);

  MCSection *MapSection;
  if (TT.getObjectFormat() == Triple::ELF) {
    if (Info.Func->hasComdat()) {
      auto ComdatName = Info.Func->getComdat()->getName();
      MapSection = (MCSection *)OutContext.getELFSection(
          MIP_MAP_SECTION_NAME, ELF::SHT_PROGBITS,
          ELF::SHF_WRITE | ELF::SHF_GROUP, 0, ComdatName,
          /*IsComdat=*/true);
    } else {
      auto GroupName = MangledName + "$MIP";
      MapSection = (MCSection *)OutContext.getELFSection(
          MIP_MAP_SECTION_NAME, ELF::SHT_PROGBITS,
          ELF::SHF_WRITE | ELF::SHF_GROUP, 0, GroupName,
          /*IsComdat=*/false);
    }
  } else {
    MapSection = OutContext.getObjectFileInfo()->getMIPMapSection();
  }
  OS.SwitchSection(MapSection);

  auto *MapEntrySymbol = OutContext.getOrCreateSymbol(MangledName + "$MAP");
  emitLinkageAndVisibility(MapEntrySymbol);
  OS.emitValueToAlignment(TT.isArch64Bit() ? 8 : 4);
  OS.emitLabel(MapEntrySymbol);

  // NOTE: Since we cannot compute a difference across sections, we use
  //       PC-relative relocations to represent the section-relative address of
  //       `Info.RawProfileSymbol`. The actual section-relative address is
  //       computed by
  //       <Raw Profile Symbol PC Offset> - (<Raw Section Start PC Offset> - C)
  //       where <Raw Section Start PC Offset> is found in the header and C is
  //       the section-relative address of this function info and is known when
  //       reading the `.mipmap` file.
  auto *ReferenceLabel = OutContext.createTempSymbol("ref");
  OS.emitLabel(ReferenceLabel);
  OS.AddComment("Raw Profile Symbol PC Offset");
  OS.emitValue(MCBinaryExpr::createSub(
                   MCSymbolRefExpr::create(Info.RawProfileSymbol, OutContext),
                   MCSymbolRefExpr::create(ReferenceLabel, OutContext),
                   OutContext),
               TT.isArch64Bit() ? 8 : 4);
  // NOTE: We use the same method to encode the offset of the function to the
  //       raw section. Then we can compute the absolute address of the function
  //       by adding the absolute address of the raw section.
  OS.AddComment("Function PC Offset");
  OS.emitValue(MCBinaryExpr::createSub(
                   MCSymbolRefExpr::create(Info.StartSymbol, OutContext),
                   MCSymbolRefExpr::create(ReferenceLabel, OutContext),
                   OutContext),
               TT.isArch64Bit() ? 8 : 4);

  OS.AddComment("Function Size");
  OS.emitAbsoluteSymbolDiff(Info.EndSymbol, Info.StartSymbol, 4);

  OS.AddComment("CFG Signature");
  OS.emitIntValueInHex(Info.ControlFlowGraphSignature, 4);

  OS.AddComment("Non-entry Block Count");
  OS.emitIntValue(Info.NonEntryBasicBlockCount, 4);

  for (uint64_t BlockID = 0; BlockID < Info.NonEntryBasicBlockCount;
       BlockID++) {
    if (Info.BasicBlockInfos.count(BlockID)) {
      const auto &MBBInfo = Info.BasicBlockInfos[BlockID];
      OS.AddComment("Block " + Twine(BlockID) + " Offset");
      OS.emitAbsoluteSymbolDiff(MBBInfo.StartSymbol, Info.StartSymbol, 4);
    } else {
      OS.emitZeros(4);
    }
  }

  OS.AddComment("Function Name Length");
  OS.emitIntValue(MangledName.size(), 4);
  OS.emitBytes(MangledName);

  OS.AddBlankLine();
}

void MIPSectionEmitter::serializeToMIPRawSection() {
  if (!MIRInstrumentation::EnableMachineInstrumentation ||
      FunctionInfos.empty())
    return;

  auto &OS = *AP.OutStreamer;
  auto &OutContext = OS.getContext();

  // NOTE: We either emit a COMDAT section or a weak definition to ensure the
  //       header symbol is deduplicated correctly.
  if (auto *MIPRawHeaderComdatSection =
          OutContext.getObjectFileInfo()->getMIPRawHeaderComdatSection()) {
    OS.SwitchSection(MIPRawHeaderComdatSection);
  } else {
    OS.SwitchSection(OutContext.getObjectFileInfo()->getMIPRawSection());
    auto *HeaderSymbol = getMIPSectionBeginSymbol(MIP_RAW_SECTION_NAME);
    OS.emitSymbolAttribute(HeaderSymbol, MCSymbolAttr::MCSA_Global);
    OS.emitSymbolAttribute(HeaderSymbol, MCSymbolAttr::MCSA_WeakDefinition);
    OS.emitSymbolAttribute(HeaderSymbol, MCSymbolAttr::MCSA_NoDeadStrip);
    OS.emitLabel(HeaderSymbol);
  }

  emitMIPHeader(MIP_FILE_TYPE_RAW);

  for (auto &Pair : FunctionInfos) {
    emitMIPFunctionData(Pair.second);
  }
}

void MIPSectionEmitter::serializeToMIPMapSection() {
  if (!MIRInstrumentation::EnableMachineInstrumentation ||
      FunctionInfos.empty())
    return;

  auto &OS = *AP.OutStreamer;
  auto &OutContext = OS.getContext();

  // NOTE: We either emit a COMDAT section or a weak definition to ensure the
  //       header symbol is deduplicated correctly.
  if (auto *MIPMapHeaderComdatSection =
          OutContext.getObjectFileInfo()->getMIPMapHeaderComdatSection()) {
    OS.SwitchSection(MIPMapHeaderComdatSection);
  } else {
    OS.SwitchSection(OutContext.getObjectFileInfo()->getMIPMapSection());
    auto *HeaderSymbol = getMIPSectionBeginSymbol(MIP_MAP_SECTION_NAME);
    OS.emitSymbolAttribute(HeaderSymbol, MCSymbolAttr::MCSA_Global);
    OS.emitSymbolAttribute(HeaderSymbol, MCSymbolAttr::MCSA_WeakDefinition);
    OS.emitLabel(HeaderSymbol);
  }

  emitMIPHeader(MIP_FILE_TYPE_MAP);

  for (auto &Pair : FunctionInfos) {
    emitMIPFunctionInfo(Pair.second);
  }
}
