//===-- ARMMCInstLower.cpp - Convert ARM MachineInstr to an MCInst --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains code to lower ARM MachineInstrs to their corresponding
// MCInst records.
//
//===----------------------------------------------------------------------===//

#include "ARM.h"
#include "ARMAsmPrinter.h"
#include "ARMBaseInstrInfo.h"
#include "ARMMachineFunctionInfo.h"
#include "ARMSubtarget.h"
#include "MCTargetDesc/ARMAddressingModes.h"
#include "MCTargetDesc/ARMBaseInfo.h"
#include "MCTargetDesc/ARMMCExpr.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/Constants.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>
#include <cstdint>

using namespace llvm;

MCOperand ARMAsmPrinter::GetSymbolRef(const MachineOperand &MO,
                                      const MCSymbol *Symbol) {
  MCSymbolRefExpr::VariantKind SymbolVariant = MCSymbolRefExpr::VK_None;
  if (MO.getTargetFlags() & ARMII::MO_SBREL)
    SymbolVariant = MCSymbolRefExpr::VK_ARM_SBREL;

  const MCExpr *Expr =
      MCSymbolRefExpr::create(Symbol, SymbolVariant, OutContext);
  switch (MO.getTargetFlags() & ARMII::MO_OPTION_MASK) {
  default:
    llvm_unreachable("Unknown target flag on symbol operand");
  case ARMII::MO_NO_FLAG:
    break;
  case ARMII::MO_LO16:
    Expr =
        MCSymbolRefExpr::create(Symbol, SymbolVariant, OutContext);
    Expr = ARMMCExpr::createLower16(Expr, OutContext);
    break;
  case ARMII::MO_HI16:
    Expr =
        MCSymbolRefExpr::create(Symbol, SymbolVariant, OutContext);
    Expr = ARMMCExpr::createUpper16(Expr, OutContext);
    break;
  }

  if (!MO.isJTI() && MO.getOffset())
    Expr = MCBinaryExpr::createAdd(Expr,
                                   MCConstantExpr::create(MO.getOffset(),
                                                          OutContext),
                                   OutContext);
  return MCOperand::createExpr(Expr);

}

bool ARMAsmPrinter::lowerOperand(const MachineOperand &MO,
                                 MCOperand &MCOp) {
  switch (MO.getType()) {
  default: llvm_unreachable("unknown operand type");
  case MachineOperand::MO_Register:
    // Ignore all implicit register operands.
    if (MO.isImplicit())
      return false;
    assert(!MO.getSubReg() && "Subregs should be eliminated!");
    MCOp = MCOperand::createReg(MO.getReg());
    break;
  case MachineOperand::MO_Immediate:
    MCOp = MCOperand::createImm(MO.getImm());
    break;
  case MachineOperand::MO_MachineBasicBlock:
    MCOp = MCOperand::createExpr(MCSymbolRefExpr::create(
        MO.getMBB()->getSymbol(), OutContext));
    break;
  case MachineOperand::MO_GlobalAddress:
    MCOp = GetSymbolRef(MO,
                        GetARMGVSymbol(MO.getGlobal(), MO.getTargetFlags()));
    break;
  case MachineOperand::MO_ExternalSymbol:
    MCOp = GetSymbolRef(MO,
                        GetExternalSymbolSymbol(MO.getSymbolName()));
    break;
  case MachineOperand::MO_JumpTableIndex:
    MCOp = GetSymbolRef(MO, GetJTISymbol(MO.getIndex()));
    break;
  case MachineOperand::MO_ConstantPoolIndex:
    if (Subtarget->genExecuteOnly())
      llvm_unreachable("execute-only should not generate constant pools");
    MCOp = GetSymbolRef(MO, GetCPISymbol(MO.getIndex()));
    break;
  case MachineOperand::MO_BlockAddress:
    MCOp = GetSymbolRef(MO, GetBlockAddressSymbol(MO.getBlockAddress()));
    break;
  case MachineOperand::MO_FPImmediate: {
    APFloat Val = MO.getFPImm()->getValueAPF();
    bool ignored;
    Val.convert(APFloat::IEEEdouble(), APFloat::rmTowardZero, &ignored);
    MCOp = MCOperand::createDFPImm(bit_cast<uint64_t>(Val.convertToDouble()));
    break;
  }
  case MachineOperand::MO_RegisterMask:
    // Ignore call clobbers.
    return false;
  }
  return true;
}

void llvm::LowerARMMachineInstrToMCInst(const MachineInstr *MI, MCInst &OutMI,
                                        ARMAsmPrinter &AP) {
  OutMI.setOpcode(MI->getOpcode());

  // In the MC layer, we keep modified immediates in their encoded form
  bool EncodeImms = false;
  switch (MI->getOpcode()) {
  default: break;
  case ARM::MOVi:
  case ARM::MVNi:
  case ARM::CMPri:
  case ARM::CMNri:
  case ARM::TSTri:
  case ARM::TEQri:
  case ARM::MSRi:
  case ARM::ADCri:
  case ARM::ADDri:
  case ARM::ADDSri:
  case ARM::SBCri:
  case ARM::SUBri:
  case ARM::SUBSri:
  case ARM::ANDri:
  case ARM::ORRri:
  case ARM::EORri:
  case ARM::BICri:
  case ARM::RSBri:
  case ARM::RSBSri:
  case ARM::RSCri:
    EncodeImms = true;
    break;
  }

  for (const MachineOperand &MO : MI->operands()) {
    MCOperand MCOp;
    if (AP.lowerOperand(MO, MCOp)) {
      if (MCOp.isImm() && EncodeImms) {
        int32_t Enc = ARM_AM::getSOImmVal(MCOp.getImm());
        if (Enc != -1)
          MCOp.setImm(Enc);
      }
      OutMI.addOperand(MCOp);
    }
  }
}

void ARMAsmPrinter::EmitSled(const MachineInstr &MI, SledKind Kind)
{
  if (MI.getParent()->getParent()->getInfo<ARMFunctionInfo>()
    ->isThumbFunction())
  {
    MI.emitError("An attempt to perform XRay instrumentation for a"
      " Thumb function (not supported). Detected when emitting a sled.");
    return;
  }
  static const int8_t NoopsInSledCount = 6;
  // We want to emit the following pattern:
  //
  // .Lxray_sled_N:
  //   ALIGN
  //   B #20
  //   ; 6 NOP instructions (24 bytes)
  // .tmpN
  //
  // We need the 24 bytes (6 instructions) because at runtime, we'd be patching
  // over the full 28 bytes (7 instructions) with the following pattern:
  //
  //   PUSH{ r0, lr }
  //   MOVW r0, #<lower 16 bits of function ID>
  //   MOVT r0, #<higher 16 bits of function ID>
  //   MOVW ip, #<lower 16 bits of address of __xray_FunctionEntry/Exit>
  //   MOVT ip, #<higher 16 bits of address of __xray_FunctionEntry/Exit>
  //   BLX ip
  //   POP{ r0, lr }
  //
  OutStreamer->emitCodeAlignment(4, &getSubtargetInfo());
  auto CurSled = OutContext.createTempSymbol("xray_sled_", true);
  OutStreamer->emitLabel(CurSled);
  auto Target = OutContext.createTempSymbol();

  // Emit "B #20" instruction, which jumps over the next 24 bytes (because
  // register pc is 8 bytes ahead of the jump instruction by the moment CPU
  // is executing it).
  // By analogy to ARMAsmPrinter::emitPseudoExpansionLowering() |case ARM::B|.
  // It is not clear why |addReg(0)| is needed (the last operand).
  EmitToStreamer(*OutStreamer, MCInstBuilder(ARM::Bcc).addImm(20)
    .addImm(ARMCC::AL).addReg(0));

  emitNops(NoopsInSledCount);

  OutStreamer->emitLabel(Target);
  recordSled(CurSled, MI, Kind, 2);
}

void ARMAsmPrinter::LowerPATCHABLE_FUNCTION_ENTER(const MachineInstr &MI)
{
  EmitSled(MI, SledKind::FUNCTION_ENTER);
}

void ARMAsmPrinter::LowerPATCHABLE_FUNCTION_EXIT(const MachineInstr &MI)
{
  EmitSled(MI, SledKind::FUNCTION_EXIT);
}

void ARMAsmPrinter::LowerPATCHABLE_TAIL_CALL(const MachineInstr &MI)
{
  EmitSled(MI, SledKind::TAIL_CALL);
}

void ARMAsmPrinter::LowerMIP_FUNCTION_INSTRUMENTATION_MARKER(
    const MachineInstr &MI) {
  MIPEmitter.runOnFunctionInstrumentationMarker(MI);
}

void ARMAsmPrinter::LowerMIP_FUNCTION_COVERAGE_INSTRUMENTATION(
    const MachineInstr &MI) {
  auto *RawProfileSymbol = MIPEmitter.getRawProfileSymbol(*MI.getMF());

  auto &AFI = *MI.getParent()->getParent()->getInfo<ARMFunctionInfo>();
  auto *RawProfileSymbolLocationLabel =
      OutContext.createTempSymbol("RawSymbolLoc", true);
  auto *LoadLabel = OutContext.createTempSymbol("LoadLabel", true);
  auto *ContinueLabel = OutContext.createTempSymbol("ContinueLabel", true);
  const MCExpr *RawProfileSymbolLocation =
      MCSymbolRefExpr::create(RawProfileSymbolLocationLabel, OutContext);

  // TODO: The emitted code is very unoptimized, but we initially strive for
  //       correctness.
  OutStreamer->AddComment("MIP: Function Coverage");
  // push   {r0, r1}
  auto PushOpcode = AFI.isThumbFunction() ? ARM::tPUSH : ARM::STMDB_UPD;
  auto PushInst = MCInstBuilder(PushOpcode);
  if (!AFI.isThumbFunction())
    PushInst.addReg(ARM::SP).addReg(ARM::SP);
  PushInst.addImm(ARMCC::AL)
      .addReg(ARM::NoRegister)
      .addReg(ARM::R0)
      .addReg(ARM::R1);
  EmitToStreamer(*OutStreamer, PushInst);

  // ldr    r1, <RawProfileSymbolLocation>
  if (AFI.isThumbFunction()) {
    EmitToStreamer(*OutStreamer, MCInstBuilder(ARM::tLDRpci)
                                     .addReg(ARM::R1)
                                     .addOperand(MCOperand::createExpr(
                                         RawProfileSymbolLocation))
                                     .addImm(ARMCC::AL)
                                     .addReg(ARM::NoRegister));
  } else {
    EmitToStreamer(*OutStreamer, MCInstBuilder(ARM::LDRi12)
                                     .addReg(ARM::R1)
                                     .addOperand(MCOperand::createExpr(
                                         RawProfileSymbolLocation))
                                     .addImm(0)
                                     .addImm(ARMCC::AL)
                                     .addReg(ARM::NoRegister));
  }

  // <LoadLabel>:
  // add    r1, pc, r1
  OutStreamer->emitLabel(LoadLabel);
  if (AFI.isThumbFunction()) {
    EmitToStreamer(*OutStreamer, MCInstBuilder(ARM::tADDhirr)
                                     .addReg(ARM::R1)
                                     .addReg(ARM::R1)
                                     .addReg(ARM::PC)
                                     .addImm(ARMCC::AL)
                                     .addReg(ARM::NoRegister));
  } else {
    EmitToStreamer(*OutStreamer, MCInstBuilder(ARM::ADDrr)
                                     .addReg(ARM::R1)
                                     .addReg(ARM::PC)
                                     .addReg(ARM::R1)
                                     .addImm(ARMCC::AL)
                                     .addReg(ARM::NoRegister)
                                     .addReg(ARM::NoRegister));
  }

  // mov    r0, #0
  if (AFI.isThumbFunction()) {
    EmitToStreamer(*OutStreamer, MCInstBuilder(ARM::tMOVi8)
                                     .addReg(ARM::R0)
                                     .addReg(ARM::CPSR)
                                     .addImm(0)
                                     .addImm(ARMCC::AL)
                                     .addReg(ARM::NoRegister));
  } else {
    EmitToStreamer(*OutStreamer, MCInstBuilder(ARM::MOVi)
                                     .addReg(ARM::R0)
                                     .addImm(0)
                                     .addImm(ARMCC::AL)
                                     .addReg(ARM::NoRegister)
                                     .addReg(ARM::NoRegister));
  }

  // strb   r0, [r1]
  if (AFI.isThumbFunction()) {
    EmitToStreamer(*OutStreamer, MCInstBuilder(ARM::tSTRBi)
                                     .addReg(ARM::R0)
                                     .addReg(ARM::R1)
                                     .addImm(0)
                                     .addImm(ARMCC::AL)
                                     .addReg(ARM::NoRegister));
  } else {
    EmitToStreamer(*OutStreamer, MCInstBuilder(ARM::STRB_POST_IMM)
                                     .addReg(ARM::R1)
                                     .addReg(ARM::R0)
                                     .addReg(ARM::R1)
                                     .addReg(ARM::NoRegister)
                                     .addImm(0)
                                     .addImm(ARMCC::AL)
                                     .addReg(ARM::NoRegister));
  }

  // pop    {r0, r1}
  auto PopOpcode = AFI.isThumbFunction() ? ARM::tPOP : ARM::LDMIA_UPD;
  auto PopInst = MCInstBuilder(PopOpcode);
  if (!AFI.isThumbFunction())
    PopInst.addReg(ARM::SP).addReg(ARM::SP);
  PopInst.addImm(ARMCC::AL)
      .addReg(ARM::NoRegister)
      .addReg(ARM::R0)
      .addReg(ARM::R1);
  EmitToStreamer(*OutStreamer, PopInst);

  // b      <ContinueLabel>
  auto BranchOpcode = AFI.isThumbFunction() ? ARM::tB : ARM::Bcc;
  EmitToStreamer(*OutStreamer, MCInstBuilder(BranchOpcode)
                                   .addExpr(MCSymbolRefExpr::create(
                                       ContinueLabel, OutContext))
                                   .addImm(ARMCC::AL)
                                   .addReg(ARM::NoRegister));
  // NOTE: T16 LDR instructions require labels to be 4 byte aligned.
  // .p2align   2
  // <RawProfileSymbolLabel>:
  // .long  <RawProfileSymbol>-(<LoadLabel>+<PCFixup>)
  OutStreamer->emitCodeAlignment(4, &getSubtargetInfo());
  OutStreamer->emitLabel(RawProfileSymbolLocationLabel);
  // NOTE: In ARM the value of PC is the address of the current instruction
  //       plus 8 bytes, but in Thumb it's 4 bytes.
  int64_t PCFixupValue = AFI.isThumbFunction() ? 4 : 8;
  const auto *PCRelativeAddress = MCBinaryExpr::createSub(
      MCSymbolRefExpr::create(RawProfileSymbol, OutContext),
      MCBinaryExpr::createAdd(MCSymbolRefExpr::create(LoadLabel, OutContext),
                              MCConstantExpr::create(PCFixupValue, OutContext),
                              OutContext),
      OutContext);
  OutStreamer->emitValue(PCRelativeAddress, 4);

  // <ContinueLabel>:
  OutStreamer->emitLabel(ContinueLabel);
}

void ARMAsmPrinter::LowerMIP_BASIC_BLOCK_COVERAGE_INSTRUMENTATION(
    const MachineInstr &MI) {
  llvm_unreachable("MIP block coverage is not implemlented for ARM targets");
}
