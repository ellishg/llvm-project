//===-------------- MIRInstrumentationPass.cpp ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "machine-ir-instrumentation"

#include "llvm/CodeGen/MIRInstrumentationPass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/InitializePasses.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

STATISTIC(NumInstrumented, "Number of machine functions instrumented");
STATISTIC(NumBlocksInstrumented, "Number of machine basic blocks instrumented");

char MIRInstrumentation::ID;
char &llvm::MIRInstrumentationID = MIRInstrumentation::ID;
INITIALIZE_PASS(MIRInstrumentation, DEBUG_TYPE,
                "Add instrumentation code to machine functions.", false, false)

cl::opt<bool> MIRInstrumentation::EnableMachineInstrumentation(
    "enable-machine-instrumentation", cl::init(false), cl::ZeroOrMore,
    cl::desc("Instrument machine ir"));
cl::opt<bool> MIRInstrumentation::EnableMachineFunctionCoverage(
    "enable-machine-function-coverage", cl::init(false), cl::ZeroOrMore,
    cl::desc("Instrument machine ir to profile function coverage only."));
cl::opt<bool> MIRInstrumentation::EnableMachineBasicBlockCoverage(
    "enable-machine-block-coverage", cl::init(false), cl::ZeroOrMore,
    cl::desc("Instrument machine ir to profile machine basic blocks."));
cl::opt<bool> MIRInstrumentation::EnableMachineCallGraph(
    "enable-machine-call-graph", cl::init(false), cl::ZeroOrMore,
    cl::desc("Instrument machine ir to profile the dynamic call graph."));
cl::opt<unsigned> MIRInstrumentation::MachineProfileRuntimeBufferSize(
    "machine-profile-runtime-buffer", cl::init(0), cl::ZeroOrMore,
    cl::value_desc("RuntimeBufferSize"),
    cl::desc("Allocate a buffer of <RuntimeBufferSize> bytes to hold machine "
             "function call samples."));
cl::opt<unsigned> MIRInstrumentation::MachineProfileFunctionGroupCount(
    "machine-profile-function-group-count", cl::init(1), cl::ZeroOrMore,
    cl::value_desc("N"),
    cl::desc(
        "Partition the machine functions into <N> groups and instrument the "
        "group specified by -machine-profile-selected-function-group."));
cl::opt<unsigned> MIRInstrumentation::MachineProfileSelectedFunctionGroup(
    "machine-profile-selected-function-group", cl::init(0), cl::ZeroOrMore,
    cl::value_desc("i"),
    cl::desc("Instrument group <i>. Must be in the range [0, "
             "-fmachine-profile-function-group-count)."));
cl::opt<unsigned> MIRInstrumentation::MachineProfileMinInstructionCount(
    "machine-profile-min-instruction-count", cl::init(0), cl::ZeroOrMore,
    cl::value_desc("N"),
    cl::desc("Do not instrument machine function that have fewer than <N> "
             "machine instructions."));

std::string MIRInstrumentation::LinkUnitName;
cl::opt<std::string, true> MIRInstrumentation::LinkUnitNameOption(
    "link-unit-name", cl::location(MIRInstrumentation::LinkUnitName),
    cl::init(""), cl::ZeroOrMore, cl::value_desc("LinkUnitName"),
    cl::desc("Use <LinkUnitName> to identify this link unit"));

bool MIRInstrumentation::doInitialization(Module &M) {
  auto &Ctx = M.getContext();
  if (EnableMachineInstrumentation) {
    if (EnableMachineFunctionCoverage == EnableMachineCallGraph)
      Ctx.emitError("Exactly one of -" + Twine(EnableMachineCallGraph.ArgStr) +
                    " or -" + Twine(EnableMachineFunctionCoverage.ArgStr) +
                    " must be provided when using -" +
                    Twine(EnableMachineInstrumentation.ArgStr) + ".");

    if (EnableMachineFunctionCoverage && MachineProfileRuntimeBufferSize)
      Ctx.emitError("Cannot set -" +
                    Twine(MachineProfileRuntimeBufferSize.ArgStr) + " when -" +
                    Twine(EnableMachineFunctionCoverage.ArgStr) +
                    " is provided.");

    if (MachineProfileRuntimeBufferSize)
      Ctx.emitError("-" + Twine(MachineProfileRuntimeBufferSize.ArgStr) +
                    " is not yet implemented.");
  }
  return false;
}

bool MIRInstrumentation::runOnMachineFunction(MachineFunction &MF) {
  if (!shouldInstrumentMachineFunction(MF))
    return false;

  LLVM_DEBUG(dbgs() << "Visit " << MF.getName());

  SmallVector<MachineBasicBlock *, 4> MBBs;
  getMachineBasicBlocks(MF, MBBs);
  if (MBBs.empty()) {
    LLVM_DEBUG(dbgs() << MF.getName() << " has zero non-debug blocks");
    return false;
  }

  auto &EntryBlock = *MBBs[0];
  unsigned NonEntryBlockCount =
      EnableMachineBasicBlockCoverage ? MBBs.size() - 1 : 0;

  const auto &TII = *MF.getSubtarget().getInstrInfo();
  auto MBBI = EntryBlock.begin();
  const auto &DL = MBBI->getDebugLoc();
  BuildMI(EntryBlock, MBBI, DL,
          TII.get(TargetOpcode::MIP_FUNCTION_INSTRUMENTATION_MARKER))
      .addImm(getControlFlowGraphSignature(MBBs))
      .addImm(NonEntryBlockCount);
  ++NumInstrumented;

  if (EnableMachineFunctionCoverage) {
    BuildMI(EntryBlock, MBBI, DL,
            TII.get(TargetOpcode::MIP_FUNCTION_COVERAGE_INSTRUMENTATION))
        .addReg(TII.getTemporaryMachineProfileRegister(EntryBlock));
  } else if (EnableMachineCallGraph) {
    BuildMI(EntryBlock, MBBI, DL, TII.get(TargetOpcode::MIP_INSTRUMENTATION))
        .addReg(TII.getTemporaryMachineProfileRegister(EntryBlock))
        .addExternalSymbol("__llvm_mip_call_counts_caller");
  } else {
    llvm_unreachable(
        "Expected function coverage or call graph instrumentation.");
  }

  for (uint32_t BlockID = 0; BlockID < NonEntryBlockCount; BlockID++) {
    auto &MBB = *MBBs[/*EntryBlock=*/1 + BlockID];
    runOnMachineBasicBlock(MBB, BlockID);
  }

  return true;
}

void MIRInstrumentation::runOnMachineBasicBlock(MachineBasicBlock &MBB,
                                                uint32_t BlockID) {
  const auto &MF = *MBB.getParent();
  const auto &TII = *MF.getSubtarget().getInstrInfo();
  auto MBBI = MBB.begin();
  const auto &DL = MBBI->getDebugLoc();
  BuildMI(MBB, MBBI, DL,
          TII.get(TargetOpcode::MIP_BASIC_BLOCK_COVERAGE_INSTRUMENTATION))
      .addReg(TII.getTemporaryMachineProfileRegister(MBB))
      .addImm(BlockID);
  ++NumBlocksInstrumented;
}

bool MIRInstrumentation::shouldInstrumentMachineFunction(
    const MachineFunction &MF) const {
  auto &F = MF.getFunction();
  auto Name = MF.getName();
  if (MF.empty() || Name.empty())
    return false;

  if (Name.startswith("OUTLINED_FUNCTION_"))
    return false;

  if (F.getCallingConv() == CallingConv::CXX_FAST_TLS)
    return false;

  if (F.hasFnAttribute(Attribute::Naked))
    return false;

  if (MF.getInstructionCount() < MachineProfileMinInstructionCount)
    return false;

  if (MachineProfileFunctionGroupCount > 1) {
    unsigned Group = MD5Hash(Name) % MachineProfileFunctionGroupCount;
    if (Group != MachineProfileSelectedFunctionGroup)
      return false;
  }

  return true;
}

void MIRInstrumentation::getMachineBasicBlocks(
    MachineFunction &MF, SmallVectorImpl<MachineBasicBlock *> &MBBs) const {
  auto ShouldSkipBlock = [](const MachineBasicBlock &MBB) {
    return MBB.empty() || MBB.getFirstNonDebugInstr() == MBB.end();
  };

  for (auto &MBB : MF) {
    if (!ShouldSkipBlock(MBB))
      MBBs.push_back(&MBB);
  }
}

uint32_t MIRInstrumentation::getControlFlowGraphSignature(
    SmallVectorImpl<MachineBasicBlock *> &MBBs) const {
  if (MBBs.size() <= 1)
    return 0;

  DenseMap<const MachineBasicBlock *, uint32_t> BlockToID;
  uint32_t ID = 0;
  for (auto *MBB : MBBs)
    BlockToID[MBB] = ID++;

  std::string AdjacencyList;
  raw_string_ostream OS(AdjacencyList);
  for (auto *MBB : MBBs) {
    OS << "{";
    for (auto *Succ : MBB->successors())
      OS << BlockToID[Succ] << ";";
    OS << "}";
  }

  return MD5Hash(OS.str());
}
