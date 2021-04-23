//===---- MIPReader.h -------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_MIP_READER_H
#define LLVM_CODEGEN_MIP_READER_H

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/MIP/MIP.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/MemoryBuffer.h"

namespace llvm {
namespace MachineProfile {

class MIPReader {
public:
  static ErrorOr<std::unique_ptr<MIRProfile>> read(const Twine &Filename);

private:
  static std::error_code readData(std::unique_ptr<MemoryBuffer> &Buffer,
                                  std::unique_ptr<MIRProfile> &MIP);
  static MFProfile readNextProfile(const char *&Data);
};

class MIPMapReader {
public:
  static ErrorOr<std::unique_ptr<MIRProfile>> read(const Twine &Filename);

private:
  static std::error_code readData(std::unique_ptr<MemoryBuffer> &Buffer,
                                  std::unique_ptr<MIRProfile> &MIP);
  static ErrorOr<std::unique_ptr<MFProfile>>
  readNextProfile(const char *&Data, uint64_t CurrentOffset,
                  const MIPHeader &Header);
};

class MIPRawReader {
public:
  static ErrorOr<std::unique_ptr<MIRRawProfile>>
  read(const Twine &Filename, const std::unique_ptr<MIRProfile> &MIP);

private:
  static std::error_code readData(std::unique_ptr<MemoryBuffer> &Buffer,
                                  std::unique_ptr<MIRRawProfile> &RawMIP,
                                  const std::unique_ptr<MIRProfile> &MIP);
};

} // namespace MachineProfile
} // namespace llvm

#endif // LLVM_CODEGEN_MIP_READER_H
