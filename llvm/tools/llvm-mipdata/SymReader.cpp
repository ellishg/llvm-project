//===- SymReader.cpp --------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Symbol Reader to correlate address to source info from symbol files.
//
//===----------------------------------------------------------------------===//

#include "SymReader.h"
#include "llvm/MIP/MIP.h"
#include "llvm/Object/MachOUniversal.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::MachineProfile;

/// @}
/// Command line options.
/// @{

namespace {

static cl::opt<std::string>
    Arch("arch", cl::init("arm64"),
         cl::desc("Dump debug information for the specified CPU "
                  "architecture only. Architectures may be specified by "
                  "name or by number."));

} // namespace
/// @}
//===----------------------------------------------------------------------===//

uint32_t SymReader::getCPUType(const MachOObjectFile &MachO) {
  if (MachO.is64Bit())
    return MachO.getHeader64().cputype;
  else
    return MachO.getHeader().cputype;
}

/// Return true if the object file has not been filtered by an --arch option.
bool SymReader::filterArch(const ObjectFile &Obj) {
  if (Arch.empty())
    return true;

  if (auto *MachO = dyn_cast<MachOObjectFile>(&Obj)) {
    unsigned Value;
    if (!StringRef(Arch).getAsInteger(0, Value))
      if (Value == getCPUType(*MachO))
        return true;

    // Match as name.
    if (MachO->getArchTriple().getArchName() == Triple(Arch).getArchName())
      return true;
  }

  return false;
}

/// If the input path is a .dSYM bundle (as created by the dsymutil tool),
/// replace it with individual entries for each of the object files inside the
/// bundle otherwise return the input path.
ErrorOr<std::vector<std::string>>
SymReader::expandBundle(const std::string &InputPath) {
  std::vector<std::string> BundlePaths;
  SmallString<256> BundlePath(InputPath);
  // Normalize input path. This is necessary to accept `bundle.dSYM/`.
  sys::path::remove_dots(BundlePath);
  // Manually open up the bundle to avoid introducing additional dependencies.
  if (sys::fs::is_directory(BundlePath) &&
      sys::path::extension(BundlePath) == ".dSYM") {
    std::error_code EC;
    sys::path::append(BundlePath, "Contents", "Resources", "DWARF");
    for (sys::fs::directory_iterator Dir(BundlePath, EC), DirEnd;
         Dir != DirEnd && !EC; Dir.increment(EC)) {
      const std::string &Path = Dir->path();
      sys::fs::file_status Status;
      if (auto EC = sys::fs::status(Path, Status))
        return EC;
      switch (Status.type()) {
      case sys::fs::file_type::regular_file:
      case sys::fs::file_type::symlink_file:
      case sys::fs::file_type::type_unknown:
        BundlePaths.push_back(Path);
        break;
      default: /*ignore*/;
      }
    }
    if (EC)
      return EC;
  }
  if (!BundlePaths.size())
    BundlePaths.push_back(InputPath);
  return BundlePaths;
}

SymReader::SymReader(ObjectFile *Obj, bool Demangle) {
  SymbolFilePath = Obj->getFileName().str();

  for (auto &Section : Obj->sections()) {
    if (auto SectionName = Section.getName()) {
      if (SectionName.get() == MIP_RAW_SECTION_NAME) {
        MIPRawSectionBeginAddress = Section.getAddress();
        break;
      }
    }
  }

  symbolize::LLVMSymbolizer::Options SymbolizerOpts;
  SymbolizerOpts.PrintFunctions =
      DILineInfoSpecifier::FunctionNameKind::LinkageName;
  SymbolizerOpts.Demangle = Demangle;
  SymbolizerOpts.DefaultArch = Arch;
  SymbolizerOpts.UseSymbolTable = false;
  SymbolizerOpts.RelativeAddresses = false;
  Symbolizer = std::make_unique<symbolize::LLVMSymbolizer>(SymbolizerOpts);
}

Expected<DIInliningInfo> SymReader::getDIInliningInfo(int64_t MIPRawOffset) {
  auto Addr = object::SectionedAddress{MIPRawSectionBeginAddress + MIPRawOffset,
                                       object::SectionedAddress::UndefSection};
  return Symbolizer->symbolizeInlinedCode(SymbolFilePath, Addr);
}

ErrorOr<std::unique_ptr<SymReader>> SymReader::create(const Twine &InputPath,
                                                      bool Demangle) {
  auto ObjsOrErr = expandBundle(InputPath.str());
  if (auto EC = ObjsOrErr.getError())
    return EC;
  auto Objs = ObjsOrErr.get();
  if (Objs.size() != 1) {
    WithColor::error() << "Cannot handle " << Objs.size() << " objects in "
                       << InputPath << "\n";
    return std::make_error_code(std::errc::invalid_argument);
  }
  auto Filename = Objs[0];

  auto BuffOrErr = MemoryBuffer::getFile(Filename);
  if (auto EC = BuffOrErr.getError())
    return EC;

  auto Buffer = std::move(BuffOrErr.get());
  auto BinOrErr = createBinary(*Buffer);
  if (auto EC = errorToErrorCode(BinOrErr.takeError()))
    return EC;

  if (auto *Obj = dyn_cast<ObjectFile>(BinOrErr->get())) {
    return std::make_unique<SymReader>(Obj, Demangle);
  } else if (auto *Fat = dyn_cast<MachOUniversalBinary>(BinOrErr->get())) {
    for (auto &ObjForArch : Fat->objects()) {
      if (auto MachOOrErr = ObjForArch.getAsObjectFile()) {
        auto &MachO = **MachOOrErr;
        if (filterArch(MachO)) {
          // Consider object file that is matched only.
          return std::make_unique<SymReader>(&MachO, Demangle);
        }
      }
    }
  }

  WithColor::error() << Filename << " has unhandled type\n";
  return std::make_error_code(std::errc::invalid_argument);
}
