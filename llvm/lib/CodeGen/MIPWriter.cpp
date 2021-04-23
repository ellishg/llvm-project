//===---- MIPWriter.cpp -----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/MIPWriter.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MD5.h"
#include <set>
#include <string>

using namespace llvm;

namespace llvm {
namespace MachineProfile {

void writeMIPHeader(raw_ostream &OS, uint16_t Version, uint16_t FileType,
                    uint32_t ProfileType, uint32_t ModuleHash) {
  support::endian::Writer Writer(OS, support::little);
  Writer.write<uint32_t>(MIP_MAGIC_VALUE);
  Writer.write(Version);
  Writer.write(FileType);
  Writer.write(ProfileType);
  Writer.write(ModuleHash);
  Writer.write<int64_t>(/*Reserved=*/0);
  Writer.write<uint32_t>(/*Reserved=*/0);
  Writer.write<uint32_t>(sizeof(MIPHeader));
}

ErrorOr<std::unique_ptr<MIPWriter>> MIPWriter::create(StringRef Filename) {
  std::error_code EC;
  std::unique_ptr<raw_ostream> OS;
  OS.reset(new raw_fd_ostream(Filename, EC, sys::fs::OF_None));
  if (EC) {
    WithColor::error() << "Unable to open " << Filename << "\n";
    return EC;
  }

  return MIPWriter::create(OS);
}

void MIPWriter::write(const std::vector<MFProfile> &Profiles, uint16_t Version,
                      uint32_t ProfileType, uint32_t ModuleHash) {
  support::endian::Writer Writer(*OutputStream, support::little);

  writeMIPHeader(Writer.OS, Version, MIP_FILE_TYPE_PROFILE, ProfileType,
                 ModuleHash);

  Writer.write<uint64_t>(Profiles.size());
  for (auto &Profile : Profiles) {
    Writer.write(Profile.FunctionSignature);
    Writer.write(Profile.RawProfileDataAddress);
    Writer.write(Profile.EncodedFunctionAddress);
    Writer.write(Profile.FunctionSize);
    Writer.write(Profile.ControlFlowGraphSignature);
    Writer.write<uint32_t>(Profile.BasicBlockProfiles.size() - 1);
    Writer.write(Profile.RawProfileCount);
    Writer.write(Profile.FunctionCallCount);
    Writer.write(Profile.FunctionOrderSum);

    // NOTE: We do not include the entry basic block.
    for (auto it = Profile.BasicBlockProfiles.begin() + 1;
         it != Profile.BasicBlockProfiles.end(); it++) {
      const auto &BlockProfile = *it;
      Writer.write(BlockProfile.Offset);
      Writer.write(BlockProfile.IsCovered);
    }

    Writer.write<uint32_t>(Profile.CallEdges.size());
    for (const auto &CallEdge : Profile.CallEdges) {
      (void)CallEdge;
      llvm_unreachable("Not implemented");
    }
  }

  std::set<std::string> FunctionNames;
  for (auto &Profile : Profiles) {
    FunctionNames.insert(Profile.FunctionName);
  }

  Writer.write<uint64_t>(FunctionNames.size());
  for (auto &FunctionName : FunctionNames) {
    Writer.OS << FunctionName << '\0';
  }
}

} // namespace MachineProfile
} // namespace llvm
