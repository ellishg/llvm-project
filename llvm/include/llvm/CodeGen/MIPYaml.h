//===---- MIPYaml.h ---------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_MIP_YAML_H
#define LLVM_CODEGEN_MIP_YAML_H

#include "llvm/MIP/MIP.h"
#include "llvm/Support/YAMLTraits.h"

LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::MachineProfile::MFProfile)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::MachineProfile::MBBProfile)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::MachineProfile::CallEdge)

namespace llvm {
namespace yaml {

// Map this value as a hex value.
template <typename HexType, typename IntType>
static inline void mapRequiredAsHex(IO &io, const char *Key, IntType &Value) {
  auto HexValue = static_cast<HexType>(Value);
  io.mapRequired(Key, HexValue);
  Value = static_cast<IntType>(HexValue);
}

template <> struct ScalarBitSetTraits<llvm::MachineProfile::MIPFileType> {
  static void bitset(IO &io, llvm::MachineProfile::MIPFileType &FileType) {
    io.bitSetCase(FileType, ".mipraw", llvm::MachineProfile::MIP_FILE_TYPE_RAW);
    io.bitSetCase(FileType, ".mipret",
                  llvm::MachineProfile::MIP_FILE_TYPE_CALL_EDGE_SAMPLES);
    io.bitSetCase(FileType, ".mipmap", llvm::MachineProfile::MIP_FILE_TYPE_MAP);
    io.bitSetCase(FileType, ".mip",
                  llvm::MachineProfile::MIP_FILE_TYPE_PROFILE);
    io.bitSetCase(FileType, "64 bit",
                  llvm::MachineProfile::MIP_FILE_TYPE_64_BIT);
    io.bitSetCase(FileType, "32 bit",
                  llvm::MachineProfile::MIP_FILE_TYPE_32_BIT);
  }
};

template <> struct ScalarBitSetTraits<llvm::MachineProfile::MIPProfileType> {
  static void bitset(IO &io,
                     llvm::MachineProfile::MIPProfileType &ProfileType) {
    io.bitSetCase(ProfileType, "Function Coverage",
                  llvm::MachineProfile::MIP_PROFILE_TYPE_FUNCTION_COVERAGE);
    io.bitSetCase(ProfileType, "Block Coverage",
                  llvm::MachineProfile::MIP_PROFILE_TYPE_BLOCK_COVERAGE);
    io.bitSetCase(ProfileType, "Function Timestamp",
                  llvm::MachineProfile::MIP_PROFILE_TYPE_FUNCTION_TIMESTAMP);
    io.bitSetCase(ProfileType, "Function Call Count",
                  llvm::MachineProfile::MIP_PROFILE_TYPE_FUNCTION_CALL_COUNT);
    io.bitSetCase(ProfileType, "Return Address",
                  llvm::MachineProfile::MIP_PROFILE_TYPE_RETURN_ADDRESS);
  }
};

template <> struct MappingTraits<llvm::MachineProfile::CallEdge> {
  static void mapping(IO &io, llvm::MachineProfile::CallEdge &Edge) {
    mapRequiredAsHex<Hex32>(io, "Section Relative Source Address",
                            Edge.SectionRelativeSourceAddress);
    io.mapRequired("Weight", Edge.Weight);
  }
};

template <> struct MappingTraits<llvm::MachineProfile::MBBProfile> {
  static void mapping(IO &io, llvm::MachineProfile::MBBProfile &Profile) {
    mapRequiredAsHex<Hex32>(io, "Offset", Profile.Offset);
    io.mapRequired("Covered", Profile.IsCovered);
  }
};

template <> struct MappingTraits<llvm::MachineProfile::MIPHeader> {
  static void mapping(IO &io, llvm::MachineProfile::MIPHeader &Header) {
    auto FileType =
        static_cast<llvm::MachineProfile::MIPFileType>(Header.FileType);
    auto ProfileType =
        static_cast<llvm::MachineProfile::MIPProfileType>(Header.ProfileType);
    io.mapRequired("File Type", FileType);
    io.mapRequired("Profile Type", ProfileType);
    Header.FileType = FileType;
    Header.ProfileType = ProfileType;

    mapRequiredAsHex<Hex32>(io, "Module Hash", Header.ModuleHash);
  }
};

template <> struct MappingTraits<llvm::DILineInfo> {
  static void mapping(IO &io, llvm::DILineInfo &Info) {
    io.mapRequired("File Name", Info.FileName);
    io.mapRequired("Function Name", Info.FunctionName);
    io.mapRequired("Line", Info.Line);
  }
};

template <> struct MappingTraits<llvm::MachineProfile::MFProfile> {
  static void mapping(IO &io, llvm::MachineProfile::MFProfile &Profile) {
    io.mapRequired("Function Name", Profile.FunctionName);
    mapRequiredAsHex<Hex64>(io, "Function Signature",
                            Profile.FunctionSignature);
    mapRequiredAsHex<Hex64>(io, "Raw Profile Data Address",
                            Profile.RawProfileDataAddress);
    mapRequiredAsHex<Hex64>(io, "Encoded Function Address",
                            Profile.EncodedFunctionAddress);
    mapRequiredAsHex<Hex32>(io, "Function Size", Profile.FunctionSize);
    mapRequiredAsHex<Hex32>(io, "Control Flow Graph Signature",
                            Profile.ControlFlowGraphSignature);
    io.mapRequired("Raw Profile Count", Profile.RawProfileCount);
    io.mapRequired("Function Call Count", Profile.FunctionCallCount);
    io.mapRequired("Function Order Sum", Profile.FunctionOrderSum);
    io.mapRequired("Basic Block Profiles", Profile.BasicBlockProfiles);
    io.mapRequired("Call Edges", Profile.CallEdges);
    io.mapOptional("Source Info", Profile.SourceInfo);
  }
};

template <>
struct MappingTraits<std::unique_ptr<llvm::MachineProfile::MIRProfile>> {
  static void mapping(IO &io,
                      std::unique_ptr<llvm::MachineProfile::MIRProfile> &MIP) {
    io.mapRequired("Header", MIP->Header);
    io.mapRequired("Profiles", MIP->Profiles);
  }
};

} // namespace yaml
} // namespace llvm

#endif // LLVM_CODEGEN_MIP_YAML_H
