//===- InstrProfCorrelator.h ------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// This file defines InstrProfCorrelator used to generate PGO profiles from
// raw profile data and debug info.
//===----------------------------------------------------------------------===//

#ifndef LLVM_PROFILEDATA_INSTRPROFCORRELATOR_H
#define LLVM_PROFILEDATA_INSTRPROFCORRELATOR_H

#include "llvm/DebugInfo/DWARF/DWARFContext.h"
#include "llvm/Object/Binary.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/ProfileData/InstrProf.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/MemoryBuffer.h"
#include <vector>

namespace llvm {

/// InstrProfCorrelator - A base class used to create raw instrumentation data
/// to their functions.
class InstrProfCorrelator {
public:
  static llvm::Expected<std::unique_ptr<InstrProfCorrelator>>
  get(const StringRef DebugInfoFilename);

  /// Construct a ProfileData vector used to correlate raw instrumentation data
  /// to their functions.
  virtual Error correlateProfileData() = 0;

  static const char *FunctionNameAttributeName;
  static const char *CFGHashAttributeName;
  static const char *NumCountersAttributeName;

  enum InstrProfCorrelatorKind { CK_32Bit, CK_64Bit };
  InstrProfCorrelator(InstrProfCorrelatorKind K) : Kind(K) {}
  virtual ~InstrProfCorrelator() {}
  InstrProfCorrelatorKind getKind() const { return Kind; }

private:
  static llvm::Expected<std::unique_ptr<InstrProfCorrelator>>
  get(std::unique_ptr<MemoryBuffer> Buffer);

  const InstrProfCorrelatorKind Kind;
};

/// InstrProfCorrelatorImpl - A child of InstrProfCorrelator with a template
/// pointer type so that the ProfileData vector can be materialized.
template <class IntPtrT>
class InstrProfCorrelatorImpl : public InstrProfCorrelator {
public:
  InstrProfCorrelatorImpl(uint64_t CounterSectionAddress, bool ShouldSwapBytes);
  static bool classof(const InstrProfCorrelator *C);

  /// Return a pointer to the underlying ProfileData vector that this class
  /// constructs.
  const RawInstrProf::ProfileData<IntPtrT> *getDataPointer() const {
    return Data.empty() ? nullptr : Data.data();
  }

  /// Return the number of ProfileData elements.
  size_t getDataSize() const { return Data.size(); }

  /// Return a pointer to the compressed names string that this class
  /// constructs.
  const char *getCompressedNamesPointer() const {
    return CompressedNames.c_str();
  }

  /// Return the number of bytes in the compressed names string.
  size_t getCompressedNamesSize() const { return CompressedNames.size(); }

  static llvm::Expected<std::unique_ptr<InstrProfCorrelatorImpl<IntPtrT>>>
  get(std::unique_ptr<MemoryBuffer> Buffer, const object::ObjectFile *Obj);

protected:
  std::vector<RawInstrProf::ProfileData<IntPtrT>> Data;
  std::string CompressedNames;
  /// The address of the __llvm_prf_cnts section.
  uint64_t CounterSectionAddress;
  /// True if target and host have different endian orders.
  bool ShouldSwapBytes;

  Error correlateProfileData() override;
  virtual void correlateProfileDataImpl() = 0;

  void addProbe(const StringRef FunctionName, uint64_t CFGHash,
                IntPtrT CounterPtr, IntPtrT FunctionPtr, uint32_t NumCounters);

private:
  InstrProfCorrelatorImpl(InstrProfCorrelatorKind Kind,
                          uint64_t CounterSectionAddress, bool ShouldSwapBytes)
      : InstrProfCorrelator(Kind), CounterSectionAddress(CounterSectionAddress),
        ShouldSwapBytes(ShouldSwapBytes){};
  std::vector<std::string> Names;

  // Byte-swap the value if necessary.
  template <class T> T maybeSwap(T Value) const {
    return ShouldSwapBytes ? sys::getSwappedBytes(Value) : Value;
  }
};

/// DwarfInstrProfCorrelator - A child of InstrProfCorrelatorImpl that takes
/// DWARF debug info as input to correlate profiles.
template <class IntPtrT>
class DwarfInstrProfCorrelator : public InstrProfCorrelatorImpl<IntPtrT> {
public:
  DwarfInstrProfCorrelator(std::unique_ptr<MemoryBuffer> Buffer,
                           std::unique_ptr<DWARFContext> DICtx,
                           uint64_t CounterSectionAddress, bool ShouldSwapBytes)
      : InstrProfCorrelatorImpl<IntPtrT>(CounterSectionAddress,
                                         ShouldSwapBytes),
        Buffer(std::move(Buffer)), DICtx(std::move(DICtx)) {}

  static std::unique_ptr<DwarfInstrProfCorrelator<IntPtrT>>
  get(std::unique_ptr<MemoryBuffer> Buffer,
      const object::ELFObjectFileBase *Obj, uint64_t CounterSectionAddress,
      bool ShouldSwapBytes);

  static std::unique_ptr<DwarfInstrProfCorrelator<IntPtrT>>
  get(std::unique_ptr<MemoryBuffer> Buffer, const object::MachOObjectFile *Obj,
      uint64_t CounterSectionAddress, bool ShouldSwapBytes);

private:
  std::unique_ptr<MemoryBuffer> Buffer;
  std::unique_ptr<DWARFContext> DICtx;

  /// Return the address of the object that the provided DIE symbolizes.
  llvm::Optional<uint64_t> getLocation(const DWARFDie &Die) const;

  /// Returns true if the provided DIE symbolizes an instrumentation probe
  /// symbol.
  static bool isDIEOfProbe(const DWARFDie &Die);

  /// Iterate over DWARF DIEs to find those that symbolize instrumentation
  /// probes and construct the ProfileData vector and CompressedNames string.
  void correlateProfileDataImpl() override;
};

} // end namespace llvm

#endif // LLVM_PROFILEDATA_INSTRPROFCORRELATOR_H
