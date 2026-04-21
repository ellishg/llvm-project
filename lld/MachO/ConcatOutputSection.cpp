//===- ConcatOutputSection.cpp --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ConcatOutputSection.h"
#include "Config.h"
#include "OutputSegment.h"
#include "SymbolTable.h"
#include "Symbols.h"
#include "SyntheticSections.h"
#include "Target.h"
#include "lld/Common/CommonLinkerContext.h"
#include "llvm/BinaryFormat/MachO.h"
#include <deque>

using namespace llvm;
using namespace llvm::MachO;
using namespace lld;
using namespace lld::macho;

MapVector<NamePair, ConcatOutputSection *> macho::concatOutputSections;

void ConcatOutputSection::addInput(ConcatInputSection *input) {
  assert(input->parent == this);
  if (inputs.empty()) {
    align = input->align;
    flags = input->getFlags();
  } else {
    align = std::max(align, input->align);
    finalizeFlags(input);
  }
  inputs.push_back(input);
}

// Branch-range extension can be implemented in two ways, either through ...
//
// (1) Branch islands: Single branch instructions (also of limited range),
//     that might be chained in multiple hops to reach the desired
//     destination. On ARM64, as 16 branch islands are needed to hop between
//     opposite ends of a 2 GiB program. LD64 uses branch islands exclusively,
//     even when it needs excessive hops.
//
// (2) Thunks: Instruction(s) to load the destination address into a scratch
//     register, followed by a register-indirect branch. Thunks are
//     constructed to reach any arbitrary address, so need not be
//     chained. Although thunks need not be chained, a program might need
//     multiple thunks to the same destination distributed throughout a large
//     program so that all call sites can have one within range.
//
// The optimal approach is to mix islands for destinations within two hops,
// and use thunks for destinations at greater distance. For now, we only
// implement thunks. TODO: Adding support for branch islands!
//
// Internally -- as expressed in LLD's data structures -- a
// branch-range-extension thunk consists of:
//
// (1) new Defined symbol for the thunk named
//     <FUNCTION>.thunk.<SEQUENCE>, which references ...
// (2) new InputSection, which contains ...
// (3.1) new data for the instructions to load & branch to the far address +
// (3.2) new Relocs on instructions to load the far address, which reference ...
// (4.1) existing Defined symbol for the real function in __text, or
// (4.2) existing DylibSymbol for the real function in a dylib
//
// Nearly-optimal thunk-placement algorithm features:
//
// * Single pass: O(n) on the number of call sites.
//
// * Accounts for the exact space overhead of thunks - no heuristics
//
// * Exploits the full range of call instructions - forward & backward
//
// Data:
//
// * DenseMap<ThunkKey, ThunkInfo> thunkMap: Maps each (referent, addend)
//   pair seen on a branch relocation to its thunk bookkeeper.
//
// * struct ThunkInfo (bookkeeper): Call instructions have limited range, and
//   distant call sites might be unable to reach the same thunk, so multiple
//   thunks are necessary to serve all call sites in a very large program. A
//   thunkInfo stores state for all thunks associated with a particular
//   function:
//     (a) thunk symbol
//     (b) input section containing stub code, and
//     (c) sequence number for the active thunk incarnation.
//   When an old thunk goes out of range, we increment the sequence number and
//   create a new thunk named <FUNCTION>.thunk.<SEQUENCE>.
//
// * A thunk consists of
//     (a) a Defined symbol pointing to
//     (b) an InputSection holding machine code (similar to a MachO stub), and
//     (c) relocs referencing the real function for fixing up the stub code.
//
// * std::vector<InputSection *> MergedInputSection::thunks: A vector parallel
//   to the inputs vector. We store new thunks via cheap vector append, rather
//   than costly insertion into the inputs vector.
//
// Control Flow:
//
// * During address assignment, MergedInputSection::finalize() examines call
//   sites by ascending address and creates thunks.  When a function is beyond
//   the range of a call site, we need a thunk. Place it at the largest
//   available forward address from the call site. Call sites increase
//   monotonically and thunks are always placed as far forward as possible;
//   thus, we place thunks at monotonically increasing addresses. Once a thunk
//   is placed, it and all previous input-section addresses are final.
//
// * ConcatInputSection::finalize() and ConcatInputSection::writeTo() merge
//   the inputs and thunks vectors (both ordered by ascending address), which
//   is simple and cheap.

DenseMap<ThunkKey, ThunkInfo, ThunkMapKeyInfo> lld::macho::thunkMap;

// Determine whether we need thunks, which depends on the target arch -- RISC
// (i.e., ARM) generally does because it has limited-range branch/call
// instructions, whereas CISC (i.e., x86) generally doesn't. RISC only needs
// thunks for programs so large that branch source & destination addresses
// might differ more than the range of branch instruction(s).
bool TextOutputSection::needsThunks() const {
  if (!target->usesThunks())
    return false;
  uint64_t isecAddr = addr;
  for (ConcatInputSection *isec : inputs)
    isecAddr = alignToPowerOf2(isecAddr, isec->align) + isec->getSize();
  // Other sections besides __text might be small enough to pass this
  // test but nevertheless need thunks for calling into other sections.
  // An imperfect heuristic to use in this case is that if a section
  // we've already processed in this segment needs thunks, so do the
  // rest.
  bool needsThunks = parent && parent->needsThunks;

  // Calculate the total size of all branch target sections
  uint64_t branchTargetsSize = in.stubs->getSize();

  // Add the size of __objc_stubs section if it exists
  if (in.objcStubs && in.objcStubs->isNeeded())
    branchTargetsSize += in.objcStubs->getSize();

  if (!needsThunks &&
      isecAddr - addr + branchTargetsSize <=
          std::min(target->backwardBranchRange, target->forwardBranchRange))
    return false;
  // Yes, this program is large enough to need thunks.
  if (parent)
    parent->needsThunks = true;
  return true;
}

void ConcatOutputSection::finalizeOne(ConcatInputSection *isec) {
  size = alignToPowerOf2(size, isec->align);
  fileSize = alignToPowerOf2(fileSize, isec->align);
  isec->outSecOff = size;
  isec->isFinal = true;
  size += isec->getSize();
  fileSize += isec->getFileSize();
}

void ConcatOutputSection::finalizeContents() {
  for (ConcatInputSection *isec : inputs)
    finalizeOne(isec);
}

void TextOutputSection::finalize() {
  if (!needsThunks()) {
    for (ConcatInputSection *isec : inputs)
      finalizeOne(isec);
    return;
  }

  uint64_t forwardBranchRange = target->forwardBranchRange;
  uint64_t backwardBranchRange = target->backwardBranchRange;
  size_t thunkSize = target->thunkSize;
  size_t thunkCount = 0;
  size_t thunkCallCount = 0;
  DenseSet<uint64_t> residentThunkPages;

  auto isTargetInRange = [&](const ConcatInputSection &isec,
                             const Relocation &r) -> bool {
    uint64_t callVA = isec.getVA() + r.offset;
    uint64_t lowVA =
        backwardBranchRange < callVA ? callVA - backwardBranchRange : 0;
    uint64_t highVA = callVA + forwardBranchRange;
    auto *funcSym = cast<Symbol *>(r.referent);
    uint64_t funcVA = resolveSymbolOffsetVA(funcSym, r.type, r.addend);
    // Check if the referent is reachable with a simple call instruction.
    return lowVA <= funcVA && funcVA <= highVA;
  };
  auto getThunkInRange = [&](const ConcatInputSection &isec,
                             const Relocation &r) -> Defined * {
    assert(!isTargetInRange(isec, r));
    uint64_t callVA = isec.getVA() + r.offset;
    uint64_t lowVA =
        backwardBranchRange < callVA ? callVA - backwardBranchRange : 0;
    uint64_t highVA = callVA + forwardBranchRange;
    auto *funcSym = cast<Symbol *>(r.referent);
    ThunkInfo &thunkInfo = thunkMap[ThunkKey{funcSym, r.addend}];
    if (thunkInfo.sym) {
      uint64_t thunkVA = thunkInfo.isec->getVA();
      if (lowVA <= thunkVA && thunkVA <= highVA)
        return thunkInfo.sym;
    }
    return nullptr;
  };
  auto createThunk = [&](const ConcatInputSection &isec, Relocation &r) {
    assert(!isTargetInRange(isec, r));
    assert(getThunkInRange(isec, r) == nullptr);
    assert(isec.isFinal);
    uint64_t highVA = isec.getVA() + r.offset + forwardBranchRange;
    if (addr + size > highVA) {
      // We can only encounter this if we have a massive section (> ~128MB) or
      // an enormous number of branch instructions within a single section
      // (> ~16M), neither of which is feasible in practice. To fix we could
      // implement branch islands when the available space for thunks become too
      // small.
      fatal("encountered a branch whose target is out of range, but there is "
            "no more space for a new thunk");
    }
    auto *funcSym = cast<Symbol *>(r.referent);
    ThunkInfo &thunkInfo = thunkMap[ThunkKey{funcSym, r.addend}];
    thunkInfo.isec =
        makeSyntheticInputSection(isec.getSegName(), isec.getName());
    thunkInfo.isec->parent = this;
    assert(thunkInfo.isec->live);

    std::string addendSuffix;
    if (r.addend != 0)
      addendSuffix = "+" + std::to_string(r.addend);
    StringRef thunkName =
        saver().save(funcSym->getName() + addendSuffix + ".thunk." +
                     std::to_string(thunkInfo.sequence++));
    if (!isa<Defined>(funcSym) || cast<Defined>(funcSym)->isExternal()) {
      r.referent = thunkInfo.sym = symtab->addDefined(
          thunkName, /*file=*/nullptr, thunkInfo.isec, /*value=*/0, thunkSize,
          /*isWeakDef=*/false, /*isPrivateExtern=*/true,
          /*isReferencedDynamically=*/false, /*noDeadStrip=*/false,
          /*isWeakDefCanBeHidden=*/false);
    } else {
      r.referent = thunkInfo.sym = make<Defined>(
          thunkName, /*file=*/nullptr, thunkInfo.isec, /*value=*/0, thunkSize,
          /*isWeakDef=*/false, /*isExternal=*/false, /*isPrivateExtern=*/true,
          /*includeInSymtab=*/true, /*isReferencedDynamically=*/false,
          /*noDeadStrip=*/false, /*isWeakDefCanBeHidden=*/false);
    }
    thunkInfo.sym->used = true;
    target->populateThunk(thunkInfo.isec, funcSym, r.addend);
    // The thunk itself bakes in the addend, so the call-site reloc must
    // branch to the thunk start with no extra offset.
    r.addend = 0;
    finalizeOne(thunkInfo.isec);
    thunks.push_back(thunkInfo.isec);
    uint64_t thunkFirstByte = thunkInfo.isec->getVA();
    uint64_t thunkLastByte = thunkFirstByte + thunkInfo.isec->getSize() - 1;
    residentThunkPages.insert(thunkFirstByte / target->getPageSize());
    residentThunkPages.insert(thunkLastByte / target->getPageSize());
    ++thunkCount;
    ++thunkCallCount;
  };

  std::deque<std::tuple<ConcatInputSection *, Relocation *, ThunkKey>>
      branchesToProcess;
  SmallVector<std::tuple<ConcatInputSection *, Relocation *, Defined *>>
      deferredBranchRedirects;
  unsigned numPendingThunkTargets = 0;

  for (auto *isec : inputs) {
    bool didCreateThunk = false;
    // If we have emitted a thunk, keep emitting a thunk until we have used up
    // 5% of the backwards branch range
    const uint64_t minThunkDistance = backwardBranchRange * 0.95;
    while (!branchesToProcess.empty()) {
      auto &[callerIsec, r, thunkKey] = branchesToProcess.front();
      assert(callerIsec->isFinal);
      auto &thunkInfo = thunkMap[thunkKey];
      if (isTargetInRange(*callerIsec, *r)) {
        if (thunkInfo.pendingBranches.erase(r))
          if (thunkInfo.pendingBranches.empty())
            --numPendingThunkTargets;
        branchesToProcess.pop_front();
        continue;
      }
      if (auto *sym = getThunkInRange(*callerIsec, *r)) {
        // The pending thunk target was already decremented when we created the
        // thunk
        deferredBranchRedirects.emplace_back(callerIsec, r, sym);
        branchesToProcess.pop_front();
        continue;
      }
      uint64_t callVA = callerIsec->getVA() + r->offset;
      uint64_t highVA = callVA + forwardBranchRange;
      uint64_t nextEnd =
          alignToPowerOf2(addr + size, isec->align) + isec->getSize();
      uint64_t thunkDistance = nextEnd - callVA;
      if (nextEnd + numPendingThunkTargets * thunkSize <= highVA)
        if (!didCreateThunk || thunkDistance < minThunkDistance)
          break;

      thunkInfo.pendingBranches.clear();
      --numPendingThunkTargets;
      createThunk(*callerIsec, *r);
      didCreateThunk = true;
      branchesToProcess.pop_front();
    }
    finalizeOne(isec);

    bool hasCallsite = llvm::any_of(isec->relocs, [](Relocation &r) {
      return target->hasAttr(r.type, RelocAttrBits::BRANCH);
    });
    if (!hasCallsite)
      continue;

    // Process relocs by ascending address, i.e., ascending offset within isec
    // FIXME: This property does not hold for object files produced by ld64's
    // `-r` mode.
    assert(is_sorted(isec->relocs, [](Relocation &a, Relocation &b) {
      return a.offset > b.offset;
    }));
    for (Relocation &r : reverse(isec->relocs)) {
      if (!target->hasAttr(r.type, RelocAttrBits::BRANCH))
        continue;
      if (isTargetInRange(*isec, r))
        continue;
      if (auto *sym = getThunkInRange(*isec, r)) {
        deferredBranchRedirects.emplace_back(isec, &r, sym);
        continue;
      }
      auto *funcSym = cast<Symbol *>(r.referent);
      ThunkKey key{funcSym, r.addend};
      branchesToProcess.emplace_back(isec, &r, key);
      auto &thunkInfo = thunkMap[key];
      if (thunkInfo.pendingBranches.empty())
        ++numPendingThunkTargets;
      thunkInfo.pendingBranches.insert(&r);
    }
  }
  for (auto [callerIsec, r, thunk] : deferredBranchRedirects) {
    if (isTargetInRange(*callerIsec, *r))
      continue;
    r->referent = thunk;
    r->addend = 0;
    ++thunkCallCount;
  }
  deferredBranchRedirects.clear();

  llvm::erase_if(branchesToProcess, [&](auto &tuple) {
    auto [callerIsec, r, thunkKey] = tuple;
    auto &thunkInfo = thunkMap[thunkKey];
    if (isTargetInRange(*callerIsec, *r) || getThunkInRange(*callerIsec, *r)) {
      if (thunkInfo.pendingBranches.erase(r))
        if (thunkInfo.pendingBranches.empty())
          --numPendingThunkTargets;
    }
    return isTargetInRange(*callerIsec, *r);
  });
  DenseSet<ThunkKey, ThunkMapKeyInfo> branchTargets;
  for (auto [callerIsec, r, thunkKey] : branchesToProcess)
    if (!getThunkInRange(*callerIsec, *r))
      branchTargets.insert(thunkKey);
  assert(branchTargets.size() == numPendingThunkTargets);

  for (auto [isec, r, thunkKey] : branchesToProcess) {
    uint64_t estimatedTextEnd =
        addr + size + numPendingThunkTargets * thunkSize;
    uint64_t estimatedStubsEnd =
        alignToPowerOf2(estimatedTextEnd, in.stubs->align) +
        in.stubs->getSize();
    if (in.objcStubs && in.objcStubs->isNeeded())
      estimatedStubsEnd =
          alignToPowerOf2(estimatedStubsEnd, in.objcStubs->align) +
          in.objcStubs->getSize();

    auto &thunkInfo = thunkMap[thunkKey];
    if (thunkInfo.pendingBranches.erase(r))
      if (thunkInfo.pendingBranches.empty())
        --numPendingThunkTargets;
    uint64_t highVA = isec->getVA() + r->offset + forwardBranchRange;
    auto *funcSym = cast<Symbol *>(r->referent);
    if ((funcSym->isInStubs() ||
         (in.objcStubs && in.objcStubs->isNeeded() &&
          ObjCStubsSection::isObjCStubSymbol(funcSym))) &&
        r->addend == 0 && estimatedStubsEnd <= highVA) {
      // The branch target can reach any section in __stubs or __objc_stubs
      assert(isec->getVA() != TargetInfo::outOfRangeVA);
      continue;
    }
    if (isTargetInRange(*isec, *r))
      continue;
    if (auto *sym = getThunkInRange(*isec, *r)) {
      r->referent = sym;
      // The thunk itself bakes in the addend, so the call-site reloc must
      // branch to the thunk start with no extra offset.
      r->addend = 0;
      ++thunkCallCount;
      continue;
    }
    createThunk(*isec, *r);
  }
  assert(numPendingThunkTargets == 0);

  if (thunkCount)
    log("Created " + Twine(thunkCount) + " (" +
        Twine(thunkCount * thunkSize / 1024) + " KB) thunks residing in " +
        Twine(residentThunkPages.size()) + " pages and updated " +
        Twine(thunkCallCount) + " branch targets");
}

void ConcatOutputSection::writeTo(uint8_t *buf) const {
  for (ConcatInputSection *isec : inputs)
    isec->writeTo(buf + isec->outSecOff);
}

void TextOutputSection::writeTo(uint8_t *buf) const {
  // Merge input sections from thunk & ordinary vectors
  size_t i = 0, ie = inputs.size();
  size_t t = 0, te = thunks.size();
  while (i < ie || t < te) {
    while (i < ie && (t == te || inputs[i]->empty() ||
                      inputs[i]->outSecOff < thunks[t]->outSecOff)) {
      inputs[i]->writeTo(buf + inputs[i]->outSecOff);
      ++i;
    }
    while (t < te && (i == ie || thunks[t]->outSecOff < inputs[i]->outSecOff)) {
      thunks[t]->writeTo(buf + thunks[t]->outSecOff);
      ++t;
    }
  }
}

void ConcatOutputSection::finalizeFlags(InputSection *input) {
  switch (sectionType(input->getFlags())) {
  default /*type-unspec'ed*/:
    // FIXME: Add additional logic here when supporting emitting obj files.
    break;
  case S_4BYTE_LITERALS:
  case S_8BYTE_LITERALS:
  case S_16BYTE_LITERALS:
  case S_CSTRING_LITERALS:
  case S_ZEROFILL:
  case S_LAZY_SYMBOL_POINTERS:
  case S_MOD_TERM_FUNC_POINTERS:
  case S_THREAD_LOCAL_REGULAR:
  case S_THREAD_LOCAL_ZEROFILL:
  case S_THREAD_LOCAL_VARIABLES:
  case S_THREAD_LOCAL_INIT_FUNCTION_POINTERS:
  case S_THREAD_LOCAL_VARIABLE_POINTERS:
  case S_NON_LAZY_SYMBOL_POINTERS:
  case S_SYMBOL_STUBS:
    flags |= input->getFlags();
    break;
  }
}

ConcatOutputSection *
ConcatOutputSection::getOrCreateForInput(const InputSection *isec) {
  NamePair names = maybeRenameSection({isec->getSegName(), isec->getName()});
  ConcatOutputSection *&osec = concatOutputSections[names];
  if (!osec) {
    if (isec->getSegName() == segment_names::text &&
        isec->getName() != section_names::gccExceptTab &&
        isec->getName() != section_names::ehFrame)
      osec = make<TextOutputSection>(names.second);
    else
      osec = make<ConcatOutputSection>(names.second);
  }
  return osec;
}

NamePair macho::maybeRenameSection(NamePair key) {
  auto newNames = config->sectionRenameMap.find(key);
  if (newNames != config->sectionRenameMap.end())
    return newNames->second;
  return key;
}
