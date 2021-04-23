//===---- MIPReader.cpp -----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/MIPReader.h"
#include "llvm/Support/MD5.h"
#include <set>
#include <string>

using namespace llvm;
using namespace llvm::support;

namespace llvm {
namespace MachineProfile {

ErrorOr<std::unique_ptr<MemoryBuffer>>
readMIPHeader(const Twine &Filename, MIPFileType FileType, MIPHeader &Header) {
  auto BufferOrErr = MemoryBuffer::getFile(Filename);
  if (auto EC = BufferOrErr.getError()) {
    WithColor::error() << "Unable to open " << Filename << "\n";
    return std::make_error_code(std::errc::invalid_argument);
  }

  auto &Buffer = BufferOrErr.get();

  if (Buffer->getBufferSize() < sizeof(MIPHeader)) {
    WithColor::error() << Filename << ": too small\n";
    WithColor::error() << "Expected: >= " << sizeof(MIPHeader) << "\n";
    WithColor::error() << "     Got:    " << Buffer->getBufferSize() << "\n";
    return std::make_error_code(std::errc::invalid_argument);
  }

  const char *Data = Buffer->getBufferStart();

  Header.Magic = endian::readNext<uint32_t, little, unaligned>(Data);
  Header.Version = endian::readNext<uint16_t, little, unaligned>(Data);
  Header.FileType = endian::readNext<uint16_t, little, unaligned>(Data);
  Header.ProfileType = endian::readNext<uint32_t, little, unaligned>(Data);
  Header.ModuleHash = endian::readNext<uint32_t, little, unaligned>(Data);
  if (Header.FileType & MIP_FILE_TYPE_32_BIT) {
    Header.RawSectionOffset =
        endian::readNext<int32_t, little, unaligned>(Data);
    (void)endian::readNext<uint32_t, little, unaligned>(Data);
  } else {
    Header.RawSectionOffset =
        endian::readNext<int64_t, little, unaligned>(Data);
  }
  Header.Reserved = endian::readNext<uint32_t, little, unaligned>(Data);
  Header.OffsetToData = endian::readNext<uint32_t, little, unaligned>(Data);

  if (Header.Magic != MIP_MAGIC_VALUE) {
    WithColor::error() << Filename << ": Invalid MIPMagic value\n"
                       << "Expected: " << Twine::utohexstr(MIP_MAGIC_VALUE)
                       << "\n"
                       << "     Got: " << Twine::utohexstr(Header.Magic)
                       << "\n";
    return std::make_error_code(std::errc::invalid_argument);
  } else if (Header.Version > MIP_VERSION) {
    WithColor::error() << Filename << ": Invalid MIP version\n"
                       << "Expected: <=" << MIP_VERSION << "\n"
                       << "     Got: " << Header.Version << "\n";
    return std::make_error_code(std::errc::invalid_argument);
  } else if ((Header.FileType & 0b1111) != FileType) {
    WithColor::error() << Filename << ": Invalid file type\n"
                       << "Expected: " << Twine::utohexstr(FileType) << "\n"
                       << "     Got: " << Twine::utohexstr(Header.FileType)
                       << "\n";
    return std::make_error_code(std::errc::invalid_argument);
  } else if ((Header.FileType & MIP_FILE_TYPE_64_BIT) &&
             (Header.FileType & MIP_FILE_TYPE_32_BIT)) {
    WithColor::error() << Filename
                       << ": File type cannot be both 64 bit and 32 bit\n"
                       << "     Got: " << Twine::utohexstr(Header.FileType)
                       << "\n";
    return std::make_error_code(std::errc::invalid_argument);
  } else if (Header.ProfileType & ~(MIP_PROFILE_TYPE_FUNCTION_COVERAGE |
                                    MIP_PROFILE_TYPE_BLOCK_COVERAGE)) {
    WithColor::error() << Filename << ": Invalid profile type\n"
                       << "Got: 0x" << Twine::utohexstr(Header.ProfileType)
                       << " \n";
    return std::make_error_code(std::errc::invalid_argument);
  } else {
    return std::move(Buffer);
  }
}

ErrorOr<std::unique_ptr<MIRProfile>> MIPReader::read(const Twine &Filename) {
  std::unique_ptr<MIRProfile> MIP(new MIRProfile());
  auto BufferOrErr =
      readMIPHeader(Filename, MIP_FILE_TYPE_PROFILE, MIP->Header);
  if (auto EC = BufferOrErr.getError())
    return EC;

  if (auto EC = readData(BufferOrErr.get(), MIP))
    return EC;

  return MIP;
}

std::error_code MIPReader::readData(std::unique_ptr<MemoryBuffer> &Buffer,
                                    std::unique_ptr<MIRProfile> &MIP) {
  const char *Data = Buffer->getBufferStart() + MIP->Header.OffsetToData;

  std::map<uint64_t, std::vector<size_t>> SignatureToProfileIndices;

  auto NumProfiles = endian::readNext<uint64_t, little, unaligned>(Data);
  for (uint64_t i = 0; i < NumProfiles; i++) {
    auto Profile = readNextProfile(Data);
    size_t Index = MIP->Profiles.size();
    if (SignatureToProfileIndices.count(Profile.FunctionSignature)) {
      SignatureToProfileIndices[Profile.FunctionSignature].push_back(Index);
    } else {
      SignatureToProfileIndices[Profile.FunctionSignature] = {Index};
    }
    MIP->Profiles.push_back(Profile);
  }

  std::set<std::string> FunctionNames;
  auto NumNames = endian::readNext<uint64_t, little, unaligned>(Data);
  for (uint64_t i = 0; i < NumNames; i++) {
    std::string Name(Data);
    Data += Name.size() + 1;
    FunctionNames.insert(Name);
  }

  for (auto &Name : FunctionNames) {
    for (auto Index : SignatureToProfileIndices[MD5Hash(Name)]) {
      MIP->Profiles[Index].FunctionName = Name;
    }
  }

  return {};
}

MFProfile MIPReader::readNextProfile(const char *&Data) {
  MFProfile Profile;
  Profile.FunctionSignature =
      endian::readNext<uint64_t, little, unaligned>(Data);
  Profile.RawProfileDataAddress =
      endian::readNext<uint64_t, little, unaligned>(Data);
  Profile.EncodedFunctionAddress =
      endian::readNext<int64_t, little, unaligned>(Data);
  Profile.FunctionSize = endian::readNext<uint32_t, little, unaligned>(Data);
  Profile.ControlFlowGraphSignature =
      endian::readNext<uint32_t, little, unaligned>(Data);
  uint32_t NonEntryBasicBlockCount =
      endian::readNext<uint32_t, little, unaligned>(Data);
  Profile.RawProfileCount = endian::readNext<uint32_t, little, unaligned>(Data);
  Profile.FunctionCallCount =
      endian::readNext<uint64_t, little, unaligned>(Data);
  Profile.FunctionOrderSum =
      endian::readNext<uint64_t, little, unaligned>(Data);

  // NOTE: The file format does not include the entry basic block in the block
  //       profile list.
  MBBProfile EntryBlockProfile(0);
  EntryBlockProfile.IsCovered =
      Profile.FunctionCallCount > 0 || Profile.RawProfileCount > 0;
  Profile.BasicBlockProfiles.push_back(EntryBlockProfile);
  for (uint32_t j = 0; j < NonEntryBasicBlockCount; j++) {
    MBBProfile BlockProfile;
    BlockProfile.Offset = endian::readNext<uint32_t, little, unaligned>(Data);
    BlockProfile.IsCovered = endian::readNext<bool, little, unaligned>(Data);
    Profile.BasicBlockProfiles.push_back(BlockProfile);
  }

  uint32_t CallEdgeCount = endian::readNext<uint32_t, little, unaligned>(Data);
  for (uint32_t j = 0; j < CallEdgeCount; j++) {
    llvm_unreachable("Not implemented yet");
  }
  return Profile;
}

ErrorOr<std::unique_ptr<MIRProfile>> MIPMapReader::read(const Twine &Filename) {
  std::unique_ptr<MIRProfile> MIP(new MIRProfile());
  auto BufferOrErr = readMIPHeader(Filename, MIP_FILE_TYPE_MAP, MIP->Header);
  if (auto EC = BufferOrErr.getError())
    return EC;

  if (auto EC = readData(BufferOrErr.get(), MIP))
    return EC;

  return MIP;
}

std::error_code MIPMapReader::readData(std::unique_ptr<MemoryBuffer> &Buffer,
                                       std::unique_ptr<MIRProfile> &MIP) {
  const char *Data = Buffer->getBufferStart() + MIP->Header.OffsetToData;

  while (Data < Buffer->getBufferEnd()) {
    uint64_t CurrentOffset = Data - Buffer->getBufferStart();
    const auto &Profile = readNextProfile(Data, CurrentOffset, MIP->Header);
    MIP->Profiles.push_back(*Profile->get());
  }

  return {};
}

ErrorOr<std::unique_ptr<MFProfile>>
MIPMapReader::readNextProfile(const char *&Data, uint64_t CurrentOffset,
                              const MIPHeader &Header) {
  std::unique_ptr<MFProfile> Profile(new MFProfile());
  bool IsArch64Bit = Header.FileType & MIP_FILE_TYPE_64_BIT;
  int64_t RelativeRawSectionStart = Header.RawSectionOffset - CurrentOffset;
  int64_t RelativeRawProfileAddress =
      IsArch64Bit ? endian::readNext<int64_t, little, unaligned>(Data)
                  : endian::readNext<int32_t, little, unaligned>(Data);
  Profile->RawProfileDataAddress =
      RelativeRawProfileAddress - RelativeRawSectionStart;
  int64_t RelativeFunctionAddress =
      IsArch64Bit ? endian::readNext<int64_t, little, unaligned>(Data)
                  : endian::readNext<int32_t, little, unaligned>(Data);
  Profile->EncodedFunctionAddress =
      RelativeFunctionAddress - RelativeRawSectionStart;
  Profile->FunctionSize = endian::readNext<uint32_t, little, unaligned>(Data);
  Profile->ControlFlowGraphSignature =
      endian::readNext<uint32_t, little, unaligned>(Data);
  uint32_t NonEntryBasicBlockCount =
      endian::readNext<uint32_t, little, unaligned>(Data);

  // Create the entry block profile.
  Profile->BasicBlockProfiles.push_back(MBBProfile());
  for (size_t i = 0; i < NonEntryBasicBlockCount; i++) {
    auto Offset = endian::readNext<uint32_t, little, unaligned>(Data);
    Profile->BasicBlockProfiles.push_back(MBBProfile(Offset));
  }
  uint32_t FunctionNameSize =
      endian::readNext<uint32_t, little, unaligned>(Data);
  Profile->FunctionName = std::string(Data, FunctionNameSize);
  if (IsArch64Bit) {
    // Align to 64 bits.
    Data = reinterpret_cast<const char *>(
        alignTo<8>(reinterpret_cast<uint64_t>(Data) + FunctionNameSize));
  } else {
    // Align to 32 bits.
    Data = reinterpret_cast<const char *>(
        alignTo<4>(reinterpret_cast<uint64_t>(Data) + FunctionNameSize));
  }

  Profile->FunctionSignature = MD5Hash(Profile->FunctionName);
  Profile->RawProfileCount = 0;
  Profile->FunctionCallCount = 0;
  Profile->FunctionOrderSum = 0;

  return std::move(Profile);
}

ErrorOr<std::unique_ptr<MIRRawProfile>>
MIPRawReader::read(const Twine &Filename,
                   const std::unique_ptr<MIRProfile> &MIP) {
  std::unique_ptr<MIRRawProfile> RawMIP(new MIRRawProfile());
  auto BufferOrErr = readMIPHeader(Filename, MIP_FILE_TYPE_RAW, RawMIP->Header);
  if (auto EC = BufferOrErr.getError())
    return EC;

  if (MIP->Header.ModuleHash != RawMIP->Header.ModuleHash) {
    WithColor::error() << Filename << ": Invalid module hash\n";
    WithColor::error() << "Expected: 0x"
                       << Twine::utohexstr(MIP->Header.ModuleHash) << "\n";
    WithColor::error() << "     Got: 0x"
                       << Twine::utohexstr(RawMIP->Header.ModuleHash) << "\n";
    return std::make_error_code(std::errc::invalid_argument);
  }

  if (auto EC = readData(BufferOrErr.get(), RawMIP, MIP))
    return EC;

  return RawMIP;
}

std::error_code MIPRawReader::readData(std::unique_ptr<MemoryBuffer> &Buffer,
                                       std::unique_ptr<MIRRawProfile> &RawMIP,
                                       const std::unique_ptr<MIRProfile> &MIP) {
  for (auto &Profile : MIP->Profiles) {
    if (Profile.RawProfileDataAddress >= Buffer->getBufferSize()) {
      WithColor::error() << "Raw profile offset too large\n";
      WithColor::error() << "Expected: < "
                         << Twine::utohexstr(Buffer->getBufferSize()) << "\n";
      WithColor::error() << "     Got:   "
                         << Twine::utohexstr(Profile.RawProfileDataAddress)
                         << "\n";
      return std::make_error_code(std::errc::invalid_argument);
    }
    const char *Data = Buffer->getBufferStart() + Profile.RawProfileDataAddress;
    MFRawProfile RawProfile;
    RawProfile.RawProfileDataAddress = Profile.RawProfileDataAddress;
    if (RawMIP->Header.ProfileType & MIP_PROFILE_TYPE_FUNCTION_COVERAGE) {
      RawProfile.IsFunctionCovered =
          (endian::readNext<uint8_t, little, unaligned>(Data) == 0x00);
    }

    if (RawMIP->Header.ProfileType & MIP_PROFILE_TYPE_BLOCK_COVERAGE) {
      // NOTE: The entry basic block profile is not in the raw file format.
      RawProfile.BasicBlockCoverage.push_back(RawProfile.IsFunctionCovered);
      for (size_t i = 1; i < Profile.BasicBlockProfiles.size(); i++) {
        bool IsBlockCovered =
            (endian::readNext<uint8_t, little, unaligned>(Data) == 0x00);
        RawProfile.BasicBlockCoverage.push_back(IsBlockCovered);
      }
    }

    RawMIP->RawProfiles.push_back(RawProfile);
  }
  return {};
}

} // namespace MachineProfile
} // namespace llvm
