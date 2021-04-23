//===---- MIPWriter.h -------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_MIP_WRITER_H
#define LLVM_CODEGEN_MIP_WRITER_H

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/MIP/MIP.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/MemoryBuffer.h"

namespace llvm {
namespace MachineProfile {

class MIPWriter {
public:
  static ErrorOr<std::unique_ptr<MIPWriter>> create(StringRef Filename);

  void write(const std::vector<MFProfile> &Profiles, uint16_t Version,
             uint32_t ProfileType, uint32_t ModuleHash);

private:
  std::unique_ptr<raw_ostream> OutputStream;

  MIPWriter(std::unique_ptr<raw_ostream> &OS) : OutputStream(std::move(OS)) {}

  static std::unique_ptr<MIPWriter> create(std::unique_ptr<raw_ostream> &OS) {
    std::unique_ptr<MIPWriter> Writer;
    Writer.reset(new MIPWriter(OS));
    return Writer;
  }
};

} // namespace MachineProfile
} // namespace llvm

#endif // LLVM_CODEGEN_MIP_WRITER_H
