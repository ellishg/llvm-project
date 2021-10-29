//===- SymReader.h ----------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVM_SYMREADER_H
#define LLVM_TOOLS_LLVM_SYMREADER_H

#include "llvm/ADT/Twine.h"
#include "llvm/DebugInfo/Symbolize/Symbolize.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {
namespace MachineProfile {

class SymReader {
  uint64_t MIPRawSectionBeginAddress;

  std::unique_ptr<symbolize::LLVMSymbolizer> Symbolizer;
  std::string SymbolFilePath;

  static ErrorOr<std::vector<std::string>>
  expandBundle(const std::string &InputPath);

  static uint32_t getCPUType(const object::MachOObjectFile &MachO);
  static bool filterArch(const object::ObjectFile &Obj, const StringRef Arch);

public:
  SymReader(object::ObjectFile *Obj, const StringRef Arch, bool Demangle);
  virtual ~SymReader() = default;

  SymReader(SymReader &) = delete;
  SymReader &operator=(SymReader &) = delete;

  static ErrorOr<std::unique_ptr<SymReader>>
  create(const Twine &Filename, const StringRef Arch, bool Demangle = true);

  Expected<DIInliningInfo> getDIInliningInfo(int64_t MIPRawOffset) const;
};

} // namespace MachineProfile
} // namespace llvm

#endif // LLVM_TOOLS_LLVM_SYMREADER_H
