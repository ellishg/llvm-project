//===- llvm-mipdata.cpp - LLVM profile data tool --------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SymReader.h"
#include "llvm/CodeGen/MIPReader.h"
#include "llvm/CodeGen/MIPWriter.h"
#include "llvm/CodeGen/MIPYaml.h"
#include "llvm/MIP/MIP.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace llvm::MachineProfile;

cl::SubCommand CreateSubCommand("create",
                                "Create an empty profile from a map file.");
cl::SubCommand MergeSubCommand("merge",
                               "Merge raw data into the specified profile.");
cl::SubCommand
    OrderSubCommand("order",
                    "List functions in the optimal order for binary layout.");
cl::SubCommand ShowSubCommand("show",
                              "Show profile data in a human-readable format.");
cl::SubCommand CoveredSubCommand("covered",
                                 "List functions that have been executed.");
cl::SubCommand MIP2YamlSubCommand("mip2yaml",
                                  "Output the profile in YAML format.");
cl::SubCommand Yaml2MIPSubCommand("yaml2mip",
                                  "Produce a profile from a YAML file.");
cl::SubCommand InfoSubCommand("info",
                              "Report statistics about the specified profile.");

cl::opt<std::string>
    ProfileFilename("profile", cl::value_desc("profile"), cl::Required,
                    cl::sub(CreateSubCommand), cl::sub(MergeSubCommand),
                    cl::sub(OrderSubCommand), cl::sub(ShowSubCommand),
                    cl::sub(CoveredSubCommand), cl::sub(MIP2YamlSubCommand),
                    cl::sub(Yaml2MIPSubCommand), cl::sub(InfoSubCommand),
                    cl::desc("The profile (.mip) to use."));
cl::alias ProfileFilenameA("p", cl::desc("Alias for --profile"),
                           cl::aliasopt(ProfileFilename));

cl::opt<std::string> MapFilename(cl::Positional, cl::Required,
                                 cl::sub(CreateSubCommand),
                                 cl::desc("<.mipmap>"));

cl::list<std::string> RawFilenames(cl::Positional, cl::OneOrMore,
                                   cl::sub(MergeSubCommand),
                                   cl::desc("<.mipraw ...>"));

cl::opt<std::string> OutputFilename("output", cl::value_desc("output"),
                                    cl::init("-"), cl::sub(ShowSubCommand),
                                    cl::sub(CoveredSubCommand),
                                    cl::sub(MIP2YamlSubCommand),
                                    cl::sub(InfoSubCommand),
                                    cl::desc("The output text file."));
cl::alias OutputFilenameA("o", cl::desc("Alias for --output"),
                          cl::aliasopt(OutputFilename));

cl::opt<std::string>
    RegexOption("regex", cl::value_desc("regex"), cl::init(".*"),
                cl::sub(ShowSubCommand), cl::sub(OrderSubCommand),
                cl::sub(CoveredSubCommand),
                cl::desc("Only process function names that match <regex>."));
cl::alias RegexOptionA("r", cl::desc("Alias for --regex"),
                       cl::aliasopt(RegexOption));

cl::opt<bool>
    StrictOption("strict", cl::sub(MergeSubCommand),
                 cl::desc("Enable strict mode. Fail on corrupt raw profiles."));

cl::opt<std::string> DebugInfoFilename(
    "debug", cl::value_desc("debug info"), cl::sub(ShowSubCommand),
    cl::sub(CoveredSubCommand), cl::sub(MIP2YamlSubCommand),
    cl::desc("Use <debug info> to include source info in the profile data."));

cl::opt<std::string> YamlFilename(cl::Positional, cl::init("-"),
                                  cl::sub(Yaml2MIPSubCommand),
                                  cl::desc("<.yaml>"));

cl::opt<bool> ShowLinesOption(
    "lines", cl::sub(CoveredSubCommand),
    cl::desc("List line numbers that are covered (requires --debug)."));
cl::opt<bool> ShowPathsOption(
    "paths", cl::sub(CoveredSubCommand),
    cl::desc("List paths that are covered (requires --debug)."));

cl::opt<std::string> Arch(
    "arch", cl::init("arm64"), cl::sub(ShowSubCommand),
    cl::sub(CoveredSubCommand), cl::sub(MIP2YamlSubCommand),
    cl::desc("Dump debug information for the specified CPU architecture only. "
             "Architectures may be specified by name or by number."));

std::error_code createMain() {
  auto MIPMapOrErr = MIPMapReader::read(MapFilename);
  if (auto EC = MIPMapOrErr.getError())
    return EC;

  const auto &MIPMap = MIPMapOrErr.get();
  const auto &Header = MIPMap->Header;

  auto WriterOrErr = MIPWriter::create(ProfileFilename);
  if (auto EC = WriterOrErr.getError())
    return EC;

  auto &Writer = WriterOrErr.get();
  Writer->write(MIPMap->Profiles, MIP_VERSION, Header.ProfileType,
                Header.ModuleHash);

  WithColor::remark() << "Wrote empty profile to " << ProfileFilename << "\n";
  return std::error_code();
}

std::error_code mergeMain() {
  auto MIPOrErr = MIPReader::read(ProfileFilename);
  if (auto EC = MIPOrErr.getError())
    return EC;

  auto &MIP = MIPOrErr.get();
  const auto &Header = MIP->Header;

  unsigned MergedRawProfileCount = 0;
  for (const auto &RawFilename : RawFilenames) {
    const auto RawMIPOrErr = MIPRawReader::read(RawFilename, MIP);
    if (auto EC = RawMIPOrErr.getError()) {
      if (StrictOption) {
        return EC;
      } else {
        continue;
      }
    }

    const auto &RawMIP = RawMIPOrErr.get();
    const auto &RawHeader = RawMIP->Header;

    std::map<uint64_t, size_t> ProfileMap;
    for (size_t i = 0; i < MIP->Profiles.size(); i++) {
      ProfileMap[MIP->Profiles[i].RawProfileDataAddress] = i;
    }

    for (auto &RawProfile : RawMIP->RawProfiles) {
      auto &Profile =
          MIP->Profiles[ProfileMap[RawProfile.RawProfileDataAddress]];

      if (RawHeader.ProfileType & MIP_PROFILE_TYPE_FUNCTION_COVERAGE) {
        if (RawProfile.IsFunctionCovered) {
          Profile.RawProfileCount++;
          Profile.FunctionCallCount =
              std::max<uint64_t>(Profile.FunctionCallCount, 1);
        }
      } else {
        if (RawProfile.IsFunctionCovered) {
          Profile.RawProfileCount++;
          Profile.FunctionCallCount += RawProfile.FunctionCallCount;
          Profile.FunctionOrderSum += RawProfile.FunctionTimestamp;
        }
      }

      if (RawHeader.ProfileType & MIP_PROFILE_TYPE_BLOCK_COVERAGE) {
        assert(RawProfile.BasicBlockCoverage.size() ==
               Profile.BasicBlockProfiles.size());
        for (unsigned BlockID = 0;
             BlockID < RawProfile.BasicBlockCoverage.size(); BlockID++) {
          Profile.BasicBlockProfiles[BlockID].IsCovered |=
              RawProfile.BasicBlockCoverage[BlockID];
        }
      }
    }
    MergedRawProfileCount++;
  }

  auto WriterOrErr = MIPWriter::create(ProfileFilename);
  if (auto EC = WriterOrErr.getError())
    return EC;

  auto &Writer = WriterOrErr.get();
  Writer->write(MIP->Profiles, MIP_VERSION, Header.ProfileType,
                Header.ModuleHash);

  WithColor::remark() << "Merged " << MergedRawProfileCount
                      << " raw profiles into " << ProfileFilename << "\n";
  return std::error_code();
}

std::error_code showMain() {
  std::error_code EC;
  raw_fd_ostream OS(OutputFilename, EC);
  if (EC)
    return EC;

  const auto MIPOrErr = MIPReader::read(ProfileFilename);
  if (auto EC = MIPOrErr.getError())
    return EC;

  std::unique_ptr<SymReader> SymReader;
  if (!DebugInfoFilename.empty()) {
    auto SymReaderOrErr = SymReader::create(DebugInfoFilename, Arch);
    if (auto EC = SymReaderOrErr.getError())
      return EC;
    SymReader = std::move(SymReaderOrErr.get());
  }

  const Regex RE(RegexOption);
  for (const auto &Profile : MIPOrErr.get()->Profiles) {
    if (!RE.match(Profile.FunctionName))
      continue;

    DILineInfo SourceInfo;
    if (SymReader) {
      auto InliningInfo =
          SymReader->getDIInliningInfo(Profile.EncodedFunctionAddress);
      if (InliningInfo && InliningInfo.get().getNumberOfFrames())
        SourceInfo = InliningInfo.get().getFrame(0);
    }

    OS << Profile.FunctionName << "\n";
    if (SourceInfo)
      OS << "  Source Info: " << SourceInfo.FileName << ":" << SourceInfo.Line
         << "\n";
    OS << "  Call Count: " << Profile.FunctionCallCount << "\n";
    if (Profile.FunctionOrderSum)
      OS << "  Order Sum: " << Profile.FunctionOrderSum << "\n";
    if (Profile.BasicBlockProfiles.size() > 1) {
      OS << "  Block Coverage:";
      for (unsigned I = 0; I < Profile.BasicBlockProfiles.size(); I++) {
        const auto &BlockProfile = Profile.BasicBlockProfiles[I];
        if (I % 8 == 0) {
          OS << "\n    ";
        }
        if (BlockProfile.IsCovered) {
          WithColor(OS, raw_ostream::RED) << " HOT ";
        } else {
          WithColor(OS, raw_ostream::CYAN) << " COLD";
        }
      }
      OS << "\n";
    }
    OS << "\n";
  }

  return std::error_code();
}

std::error_code orderMain() {
  std::error_code EC;
  raw_fd_ostream OS(OutputFilename, EC);
  if (EC)
    return EC;

  const auto MIPOrErr = MIPReader::read(ProfileFilename);
  if (auto EC = MIPOrErr.getError())
    return EC;

  std::vector<MFProfile> OrderedProfiles;
  MIPOrErr.get()->getOrderedProfiles(OrderedProfiles);
  for (const auto &Profile : OrderedProfiles) {
    OS << Profile.FunctionName << "\n";
  }

  WithColor::remark() << "Ordered " << OrderedProfiles.size() << " functions\n";
  return std::error_code();
}

std::error_code coveredMain() {
  std::error_code EC;
  raw_fd_ostream OS(OutputFilename, EC);
  if (EC)
    return EC;

  const auto MIPOrErr = MIPReader::read(ProfileFilename);
  if (auto EC = MIPOrErr.getError())
    return EC;

  std::unique_ptr<SymReader> SymReader;
  if (!DebugInfoFilename.empty()) {
    auto SymReaderOrErr = SymReader::create(DebugInfoFilename, Arch);
    if (auto EC = SymReaderOrErr.getError())
      return EC;
    SymReader = std::move(SymReaderOrErr.get());
  }

  if ((ShowPathsOption || ShowLinesOption) && DebugInfoFilename.empty()) {
    WithColor::error() << "--" << ShowPathsOption.ArgStr << " and --"
                       << ShowLinesOption.ArgStr << " require <"
                       << DebugInfoFilename.ValueStr << ">\n";
    return std::make_error_code(std::errc::invalid_argument);
  } else if (ShowPathsOption && ShowLinesOption) {
    WithColor::error() << "--" << ShowPathsOption.ArgStr << " and --"
                       << ShowLinesOption.ArgStr << " cannot both be set\n";
    return std::make_error_code(std::errc::invalid_argument);
  }

  unsigned NumCoveredProfiles = 0;
  const Regex RE(RegexOption);
  for (const auto &Profile : MIPOrErr.get()->Profiles) {
    if (Profile.RawProfileCount) {
      if (RE.match(Profile.FunctionName)) {
        if (SymReader) {
          auto SourceInfoOrErr =
              SymReader->getDIInliningInfo(Profile.EncodedFunctionAddress);
          if (!SourceInfoOrErr || !SourceInfoOrErr.get().getNumberOfFrames()) {
            WithColor::warning()
                << "No debug info found for " << Profile.FunctionName << "\n";
          } else {
            auto SourceInfo = SourceInfoOrErr.get();
            for (unsigned I = 0; I < SourceInfo.getNumberOfFrames(); I++) {
              if (ShowPathsOption) {
                OS << SourceInfo.getFrame(I).FileName << "\n";
              } else if (ShowLinesOption) {
                OS << SourceInfo.getFrame(I).FileName << ":"
                   << SourceInfo.getFrame(I).Line << "\n";
              } else {
                OS << SourceInfo.getFrame(I).FunctionName << "\n";
              }
            }
          }
        } else { // No SymReader
          OS << Profile.FunctionName << "\n";
        }
      }
      NumCoveredProfiles++;
    }
  }

  WithColor::remark() << NumCoveredProfiles << " total functions are covered\n";
  return std::error_code();
}

std::error_code mip2yamlMain() {
  std::error_code EC;
  raw_fd_ostream OS(OutputFilename, EC);
  if (EC)
    return EC;

  auto MIPOrErr = MIPReader::read(ProfileFilename);
  if (auto EC = MIPOrErr.getError())
    return EC;

  auto MIP = std::move(MIPOrErr.get());
  if (!DebugInfoFilename.empty()) {
    auto SymReaderOrErr = SymReader::create(DebugInfoFilename, Arch);
    if (auto EC = SymReaderOrErr.getError())
      return EC;
    auto SymReader = std::move(SymReaderOrErr.get());
    for (auto &Profile : MIP->Profiles) {
      auto InliningInfo =
          SymReader->getDIInliningInfo(Profile.EncodedFunctionAddress);
      if (InliningInfo && InliningInfo.get().getNumberOfFrames())
        Profile.SourceInfo = InliningInfo.get().getFrame(0);
    }
  }

  yaml::Output yamlOS(OS);
  yamlOS << MIP;

  return std::error_code();
}

std::error_code yaml2mipMain() {
  auto BufferOrErr = MemoryBuffer::getFileOrSTDIN(YamlFilename);
  if (auto EC = BufferOrErr.getError()) {
    WithColor::error() << "Unable to open " << YamlFilename << "\n";
    return std::make_error_code(std::errc::invalid_argument);
  }
  auto &Buffer = BufferOrErr.get();

  yaml::Input yamlOS(Buffer->getMemBufferRef());
  std::unique_ptr<MIRProfile> MIP(new MIRProfile());
  yamlOS >> MIP;

  if (yamlOS.error()) {
    WithColor::error() << "Unable to read " << YamlFilename
                       << " as a profile\n";
    return std::make_error_code(std::errc::invalid_argument);
  }

  auto WriterOrErr = MIPWriter::create(ProfileFilename);
  if (auto EC = WriterOrErr.getError())
    return EC;

  auto &Writer = WriterOrErr.get();
  Writer->write(MIP->Profiles, MIP_VERSION, MIP->Header.ProfileType,
                MIP->Header.ModuleHash);

  WithColor::remark() << "Wrote empty profile to " << ProfileFilename << "\n";
  return std::error_code();
}

std::error_code infoMain() {
  std::error_code EC;
  raw_fd_ostream OS(OutputFilename, EC);
  if (EC)
    return EC;

  auto MIPOrErr = MIPReader::read(ProfileFilename);
  if (auto EC = MIPOrErr.getError())
    return EC;

  auto MIP = std::move(MIPOrErr.get());

  unsigned TotalFunctions = MIP->Profiles.size();
  unsigned ProfiledFunctions = std::count_if(
      MIP->Profiles.begin(), MIP->Profiles.end(),
      [](const auto &Profile) { return Profile.RawProfileCount > 0; });
  unsigned TotalRawProfiles = 0, TotalBlocks = 0, CoveredBlocks = 0,
           TotalCallEdges = 0;
  for (const auto &Profile : MIP->Profiles) {
    TotalRawProfiles = std::max(Profile.RawProfileCount, TotalRawProfiles);
    TotalBlocks += Profile.BasicBlockProfiles.size();
    CoveredBlocks += std::count_if(
        Profile.BasicBlockProfiles.begin(), Profile.BasicBlockProfiles.end(),
        [](const auto &BlockProfile) { return BlockProfile.IsCovered; });
    TotalCallEdges += Profile.CallEdges.size();
  }

  OS << "Total Raw Profile Count: " << TotalRawProfiles << "\n";
  OS << "Total Machine Functions: " << TotalFunctions << "\n";
  OS << "Profiled Machine Functions: " << ProfiledFunctions << "\n";
  OS << "Total Machine Basic Blocks: " << TotalBlocks << "\n";
  OS << "Covered Machine Basic Blocks: " << CoveredBlocks << "\n";
  OS << "Total Call Edges: " << TotalCallEdges << "\n";

  return std::error_code();
}

int main(int argc, const char *argv[]) {
  InitLLVM X(argc, argv);

  cl::SetVersionPrinter(
      [](raw_ostream &OS) { OS << "MIP Version " << MIP_VERSION << "\n"; });
  cl::ParseCommandLineOptions(argc, argv,
                              "A tool to create, populate, and read machine "
                              "instrumentation profiles (MIP).");

  std::error_code EC;
  if (CreateSubCommand) {
    EC = createMain();
  } else if (MergeSubCommand) {
    EC = mergeMain();
  } else if (ShowSubCommand) {
    EC = showMain();
  } else if (OrderSubCommand) {
    EC = orderMain();
  } else if (CoveredSubCommand) {
    EC = coveredMain();
  } else if (MIP2YamlSubCommand) {
    EC = mip2yamlMain();
  } else if (Yaml2MIPSubCommand) {
    EC = yaml2mipMain();
  } else if (InfoSubCommand) {
    EC = infoMain();
  } else {
    cl::PrintHelpMessage();
  }

  if (EC) {
    WithColor::error() << EC.message() << "\n";
    return 1;
  }

  return 0;
}
