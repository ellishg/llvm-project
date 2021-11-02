//===-- InstrProfCorrelator.cpp -------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/ProfileData/InstrProfCorrelator.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "correlator"

using namespace llvm;

/// Get the address of the __llvm_prf_cnts section.
Expected<uint64_t> getCounterSectionAddress(const object::ObjectFile &Obj) {
  for (auto &Section : Obj.sections())
    if (auto SectionName = Section.getName())
      if (SectionName.get() == INSTR_PROF_CNTS_SECT_NAME)
        return Section.getAddress();
  return make_error<InstrProfError>(
      instrprof_error::unable_to_correlate_profile);
}

const char *InstrProfCorrelator::FunctionNameAttributeName = "Function Name";
const char *InstrProfCorrelator::CFGHashAttributeName = "CFG Hash";
const char *InstrProfCorrelator::NumCountersAttributeName = "Num Counters";

llvm::Expected<std::unique_ptr<InstrProfCorrelator>>
InstrProfCorrelator::get(const StringRef DebugInfoFilename) {
  auto BufferOrErr =
      errorOrToExpected(MemoryBuffer::getFile(DebugInfoFilename));
  if (auto Err = BufferOrErr.takeError())
    return std::move(Err);

  return get(std::move(*BufferOrErr));
}

llvm::Expected<std::unique_ptr<InstrProfCorrelator>>
InstrProfCorrelator::get(std::unique_ptr<MemoryBuffer> Buffer) {
  auto BinOrErr = object::createBinary(*Buffer);
  if (auto Err = BinOrErr.takeError())
    return std::move(Err);

  if (auto *Obj = dyn_cast<object::ObjectFile>(BinOrErr->get())) {
    auto T = Obj->makeTriple();
    if (T.isArch64Bit())
      return InstrProfCorrelatorImpl<uint64_t>::get(std::move(Buffer), Obj);
    if (T.isArch32Bit())
      return InstrProfCorrelatorImpl<uint32_t>::get(std::move(Buffer), Obj);
  }
  return make_error<InstrProfError>(
      instrprof_error::unable_to_correlate_profile);
}

template <>
InstrProfCorrelatorImpl<uint32_t>::InstrProfCorrelatorImpl(
    uint64_t CounterSectionAddress, bool ShouldSwapBytes)
    : InstrProfCorrelatorImpl(InstrProfCorrelatorKind::CK_32Bit,
                              CounterSectionAddress, ShouldSwapBytes) {}
template <>
InstrProfCorrelatorImpl<uint64_t>::InstrProfCorrelatorImpl(
    uint64_t CounterSectionAddress, bool ShouldSwapBytes)
    : InstrProfCorrelatorImpl(InstrProfCorrelatorKind::CK_64Bit,
                              CounterSectionAddress, ShouldSwapBytes) {}
template <>
bool InstrProfCorrelatorImpl<uint32_t>::classof(const InstrProfCorrelator *C) {
  return C->getKind() == InstrProfCorrelatorKind::CK_32Bit;
}
template <>
bool InstrProfCorrelatorImpl<uint64_t>::classof(const InstrProfCorrelator *C) {
  return C->getKind() == InstrProfCorrelatorKind::CK_64Bit;
}

template <class IntPtrT>
llvm::Expected<std::unique_ptr<InstrProfCorrelatorImpl<IntPtrT>>>
InstrProfCorrelatorImpl<IntPtrT>::get(std::unique_ptr<MemoryBuffer> Buffer,
                                      const object::ObjectFile *Obj) {
  auto CounterSectionAddress = getCounterSectionAddress(*Obj);
  bool ShouldSwapBytes = Obj->isLittleEndian() != sys::IsLittleEndianHost;
  if (auto Err = CounterSectionAddress.takeError())
    return std::move(Err);
  if (const auto *EObj = dyn_cast<object::ELFObjectFileBase>(Obj))
    return DwarfInstrProfCorrelator<IntPtrT>::get(
        std::move(Buffer), EObj, *CounterSectionAddress, ShouldSwapBytes);
  if (const auto *MObj = dyn_cast<object::MachOObjectFile>(Obj))
    return DwarfInstrProfCorrelator<IntPtrT>::get(
        std::move(Buffer), MObj, *CounterSectionAddress, ShouldSwapBytes);
  return make_error<InstrProfError>(
      instrprof_error::unable_to_correlate_profile);
}

template <class IntPtrT>
Error InstrProfCorrelatorImpl<IntPtrT>::correlateProfileData() {
  assert(Data.empty() && CompressedNames.empty() && Names.empty());
  correlateProfileDataImpl();
  auto Result =
      collectPGOFuncNameStrings(Names, /*doCompression=*/true, CompressedNames);
  Names.clear();
  return Result;
}

template <class IntPtrT>
void InstrProfCorrelatorImpl<IntPtrT>::addProbe(const StringRef FunctionName,
                                                uint64_t CFGHash,
                                                IntPtrT CounterPtr,
                                                IntPtrT FunctionPtr,
                                                uint32_t NumCounters) {
  Data.push_back({
      maybeSwap<uint64_t>(IndexedInstrProf::ComputeHash(FunctionName)),
      maybeSwap<uint64_t>(CFGHash),
      maybeSwap<IntPtrT>(CounterPtr),
      maybeSwap<IntPtrT>(FunctionPtr),
      // TODO: Value profiling is not yet supported.
      /*ValuesPtr=*/maybeSwap<IntPtrT>(0),
      maybeSwap<uint32_t>(NumCounters),
      /*NumValueSites=*/{maybeSwap<uint16_t>(0), maybeSwap<uint16_t>(0)},
  });
  Names.push_back(FunctionName.str());
}

template <class IntPtrT>
std::unique_ptr<DwarfInstrProfCorrelator<IntPtrT>>
DwarfInstrProfCorrelator<IntPtrT>::get(std::unique_ptr<MemoryBuffer> Buffer,
                                       const object::ELFObjectFileBase *Obj,
                                       uint64_t CounterSectionAddress,
                                       bool ShouldSwapBytes) {
  auto DICtx = DWARFContext::create(*Obj);
  return std::make_unique<DwarfInstrProfCorrelator<IntPtrT>>(
      std::move(Buffer), std::move(DICtx), CounterSectionAddress,
      ShouldSwapBytes);
}

template <class IntPtrT>
std::unique_ptr<DwarfInstrProfCorrelator<IntPtrT>>
DwarfInstrProfCorrelator<IntPtrT>::get(std::unique_ptr<MemoryBuffer> Buffer,
                                       const object::MachOObjectFile *Obj,
                                       uint64_t CounterSectionAddress,
                                       bool ShouldSwapBytes) {
  assert(false && "Not yet implemented");
}

template <class IntPtrT>
llvm::Optional<uint64_t>
DwarfInstrProfCorrelator<IntPtrT>::getLocation(const DWARFDie &Die) const {
  auto Locations = Die.getLocations(dwarf::DW_AT_location);
  if (!Locations) {
    consumeError(Locations.takeError());
    return {};
  }
  auto &DU = *Die.getDwarfUnit();
  for (auto &Location : *Locations) {
    auto AddressSize = DU.getAddressByteSize();
    DataExtractor Data(Location.Expr, DICtx->isLittleEndian(), AddressSize);
    DWARFExpression Expr(Data, AddressSize);
    for (auto &Op : Expr)
      if (Op.getCode() == dwarf::DW_OP_addr)
        return Op.getRawOperand(0);
  }
  return {};
}

template <class IntPtrT>
bool DwarfInstrProfCorrelator<IntPtrT>::isDIEOfProbe(const DWARFDie &Die) {
  if (const char *Name = Die.getName(DINameKind::ShortName))
    return StringRef(Name).startswith(getInstrProfCountersVarPrefix());
  return false;
}

template <class IntPtrT>
void DwarfInstrProfCorrelator<IntPtrT>::correlateProfileDataImpl() {
  auto maybeAddProbe = [&](DWARFDie Die) {
    if (!isDIEOfProbe(Die))
      return;
    Optional<const char *> FunctionName;
    Optional<uint64_t> CFGHash;
    Optional<uint64_t> CounterPtr = getLocation(Die);
    // TODO: Warn or fail when the function address is unavailable.
    uint64_t FunctionPtr =
        dwarf::toAddress(Die.getParent().find(dwarf::DW_AT_low_pc))
            .getValueOr(0);
    Optional<uint64_t> NumCounters;
    for (const DWARFDie &Child : Die.children()) {
      if (Child.getTag() != dwarf::DW_TAG_LLVM_annotation)
        continue;
      auto AnnotationFormName = Child.find(dwarf::DW_AT_name);
      auto AnnotationFormValue = Child.find(dwarf::DW_AT_const_value);
      if (!AnnotationFormName || !AnnotationFormValue)
        continue;
      StringRef AnnotationName = *AnnotationFormName->getAsCString();
      if (AnnotationName.compare(
              InstrProfCorrelator::FunctionNameAttributeName) == 0)
        FunctionName = AnnotationFormValue->getAsCString();
      if (AnnotationName.compare(InstrProfCorrelator::CFGHashAttributeName) ==
          0)
        CFGHash = AnnotationFormValue->getAsUnsignedConstant();
      if (AnnotationName.compare(
              InstrProfCorrelator::NumCountersAttributeName) == 0)
        NumCounters = AnnotationFormValue->getAsUnsignedConstant();
    }
    if (!FunctionName || !CFGHash || !CounterPtr || !NumCounters) {
      LLVM_DEBUG(llvm::dbgs()
                 << "Incomplete DIE for probe\n\tFunctionName: " << FunctionName
                 << "\n\tCFGHash: " << CFGHash << "\n\tCounterPtr: "
                 << CounterPtr << "\n\tNumCounters: " << NumCounters);
      LLVM_DEBUG(Die.dump(llvm::dbgs()));
      return;
    }
    this->addProbe(*FunctionName, *CFGHash, *CounterPtr, FunctionPtr,
                   *NumCounters);
  };
  for (auto &CU : DICtx->normal_units())
    for (const auto &Entry : CU->dies())
      maybeAddProbe(DWARFDie(CU.get(), &Entry));
  for (auto &CU : DICtx->dwo_units())
    for (const auto &Entry : CU->dies())
      maybeAddProbe(DWARFDie(CU.get(), &Entry));
}
