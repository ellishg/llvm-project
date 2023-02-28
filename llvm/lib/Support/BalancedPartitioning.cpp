//===- BalancedPartitioning.cpp -------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements a recursive balanced graph partitioning algorithm.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/BalancedPartitioning.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
#define DEBUG_TYPE "balanced-partitioning"

void Document::dump(raw_ostream &OS, InstrProfSymtab *Symtab) {
  if (Symtab) {
    OS << Symtab->getFuncName(Id) << "\n";
  } else {
    OS << "ID: 0x" << Twine::utohexstr(Id) << "\n";
  }
  OS << "Adjacent Kmers: ";
  for (auto &Kmer : AdjacentKmers)
    OS << Kmer << " ";
  OS << "\n";
  if (Bucket)
    OS << "Bucket: " << *Bucket << "\n";
}

std::vector<Document>
Document::fromTraces(const SmallVectorImpl<std::vector<uint64_t>> &Traces) {
  // Collect all function IDs ordered by their smallest timestamp. This will be
  // used as the initial document order.
  SetVector<DocumentIDT> FunctionIds;
  for (size_t Timestamp = 0;; Timestamp++) {
    bool Finished = true;
    for (auto &Trace : Traces) {
      if (Timestamp < Trace.size()) {
        FunctionIds.insert(Trace[Timestamp]);
        Finished = false;
      }
    }
    if (Finished)
      break;
  }

  // TODO: Create cutoffs option
  // std::vector<uint32_t> Cutoffs = {1000, 2000, 4000, 8000, 15000, 20000,
  // 30000};
  std::vector<uint64_t> Cutoffs = {0, 1, 2, 3};

  DenseMap<DocumentIDT, DenseSet<KmerT>> FuncGroups;
  for (size_t TraceIdx = 0; TraceIdx < Traces.size(); TraceIdx++) {
    auto &Trace = Traces[TraceIdx];
    for (size_t Timestamp = 0; Timestamp < Trace.size(); Timestamp++) {
      for (size_t CutoffIdx = 0; CutoffIdx < Cutoffs.size(); CutoffIdx++) {
        if (Timestamp <= Cutoffs[CutoffIdx]) {
          DocumentIDT FunctionId = Trace[Timestamp];
          // TODO: Make a KmerHashT.
          KmerT GroupId = TraceIdx * Cutoffs.size() + CutoffIdx;
          FuncGroups[FunctionId].insert(GroupId);
        }
      }
    }
  }
  std::vector<Document> Documents;
  for (auto &Id : FunctionIds)
    Documents.emplace_back(Id, FuncGroups[Id]);
  return Documents;
}

BalancedPartitioning::BalancedPartitioning(
    const BalancedPartitioningConfig &Config)
    : Config(Config), ThreadPoolPtr(std::make_unique<ThreadPool>()) {
  // Pre-computing log2 values
  Log2Cache[0] = 0.0;
  for (uint32_t I = 1; I < LOG_CACHE_SIZE; I++)
    Log2Cache[I] = std::log2(I);
}

void BalancedPartitioning::run(std::vector<Document> &Documents) const {
  LLVM_DEBUG({
    dbgs() << "[FunctionSimilarityOrderer] Partitioning " << Documents.size()
           << " documents using depth " << Config.SplitDepth << " and "
           << Config.IterationsPerSplit << " iterations per split\n";
  });

  // Record the input document order
  for (unsigned I = 0; I < Documents.size(); I++)
    Documents[I].InputOrder = I;

  // TODO
  // ThreadPoolPtr->async([&] { bisect(begin(Documents), end(Documents), 0, 1,
  // 0); }); Pool.wait() is not working, as tasks are added during recursive
  // computation
  // Pool->await();
  bisect(llvm::make_range(Documents.begin(), Documents.end()), 0, 1, 0);

  llvm::stable_sort(Documents, [](const auto &L, const auto &R) {
    return L.Bucket < R.Bucket;
  });

  LLVM_DEBUG({
    dbgs() << "[FunctionSimilarityOrderer] Balanced partitioning completed\n";
  });
}

void BalancedPartitioning::bisect(const DocumentRange Docs, uint32_t RecDepth,
                                  uint32_t RootBucket, uint32_t Offset) const {
  uint32_t NumDocuments = std::distance(Docs.begin(), Docs.end());
  if (NumDocuments < 1 || RecDepth >= Config.SplitDepth) {
    // We've reach the lowest level of the recursion tree. Fall back to the
    // original order and assign to buckets.
    llvm::stable_sort(Docs, [](const auto &L, const auto &R) {
      return L.InputOrder < R.InputOrder;
    });
    for (auto &Doc : Docs)
      Doc.Bucket = Offset++;
    return;
  }

  LLVM_DEBUG(dbgs() << "Bisect with " << NumDocuments
                    << " documents and root bucket " << RootBucket << "\n");

  std::mt19937 RNG(RootBucket);

  uint32_t LeftBucket = 2 * RootBucket;
  uint32_t RightBucket = 2 * RootBucket + 1;

  // Split into two and assign to the left and right buckets
  split(Docs, LeftBucket);

  bool Improvement =
      runIterations(Docs, RecDepth, LeftBucket, RightBucket, RNG);
  if (!Improvement) {
    // No improvement, so assign buckets and stop
    for (auto &Doc : Docs)
      Doc.Bucket = Offset++;
    return;
  }

  // Split documents wrt the resulting buckets
  auto DocsMid = llvm::partition(
      Docs, [LeftBucket](auto &Doc) { return Doc.Bucket == LeftBucket; });

  uint32_t MidOffset = Offset + std::distance(Docs.begin(), DocsMid);

  // Two recursive tasks
  auto leftRecTask = [=]() {
    bisect(llvm::make_range(Docs.begin(), DocsMid), RecDepth + 1, LeftBucket,
           Offset);
  };
  auto rightRecTask = [=]() {
    bisect(llvm::make_range(DocsMid, Docs.end()), RecDepth + 1, RightBucket,
           MidOffset);
  };

  // if (RecDepth < TaskSplitDepth && NumDocuments >= 4) {
  //   // Start the subtasks in parallel, if we have enough documents to process
  //   Pool->async(std::move(leftRecTask));
  //   Pool->async(std::move(rightRecTask));
  // } else {
  // Run recursively on the same thread
  leftRecTask();
  rightRecTask();
  // }
}

bool BalancedPartitioning::runIterations(const DocumentRange Docs,
                                         uint32_t RecDepth, uint32_t LeftBucket,
                                         uint32_t RightBucket,
                                         std::mt19937 &RNG) const {
  uint32_t NumDocuments = std::distance(Docs.begin(), Docs.end());

  // Initialize signatures. Empirically the number of corresponding k-mers is
  // 4x larger than the number of documents.
  SignaturesType Signatures;
  Signatures.reserve(NumDocuments * 4);
  initializeSignatures(Signatures, Docs, LeftBucket);

  double InitialGoal = computeGoal(Signatures);

  for (uint32_t Iter = 0; Iter < Config.IterationsPerSplit; Iter++) {
    uint32_t NumMovedDocuments =
        runIteration(Docs, LeftBucket, RightBucket, Signatures, RNG);
    if (NumMovedDocuments == 0)
      break;
  }

  double FinalGoal = computeGoal(Signatures);
  // Return true iff progress has been made
  return InitialGoal > FinalGoal;
}

void BalancedPartitioning::initializeSignatures(SignaturesType &Signatures,
                                                const DocumentRange Docs,
                                                uint32_t LeftBucket) const {
  for (auto &Doc : Docs) {
    for (auto &Kmer : Doc.AdjacentKmers) {
      if (Doc.Bucket == LeftBucket) {
        Signatures[Kmer].LeftCount++;
      } else {
        Signatures[Kmer].RightCount++;
      }
    }
  }
}

uint32_t BalancedPartitioning::runIteration(const DocumentRange Docs,
                                            uint32_t LeftBucket,
                                            uint32_t RightBucket,
                                            SignaturesType &Signatures,
                                            std::mt19937 &RNG) const {
  // Init signature caches, if needed
  for (auto &[Kmer, Signature] : Signatures) {
    if (Signature.CacheIsInvalid) {
      prepareSignature(Signature);
      Signature.CacheIsInvalid = false;
    }
  }

  // Compute move gains
  typedef std::pair<double, Document *> GainPair;
  std::vector<GainPair> Gains;
  for (auto &Doc : Docs) {
    bool FromLeftToRight = (Doc.Bucket == LeftBucket);
    double Gain = moveGain(Doc, FromLeftToRight, Signatures);
    Gains.push_back(std::make_pair(Gain, &Doc));
  }

  // Collect left and right gains
  auto LeftGains = Gains.begin();
  auto LeftEnd = llvm::partition(
      Gains, [&](const auto &GP) { return GP.second->Bucket == LeftBucket; });

  auto RightGains = LeftEnd;
  auto RightEnd = Gains.end();

  // Sort gains
  auto LargerGain = [](const auto &L, const auto &R) {
    return L.first > R.first;
  };
  std::stable_sort(LeftGains, LeftEnd, LargerGain);
  std::stable_sort(RightGains, RightEnd, LargerGain);

  // Exchange: change buckets and update queryVertex signatures
  uint32_t NumMovedDataVertices = 0;
  uint32_t MinSize = std::min(std::distance(LeftGains, LeftEnd),
                              std::distance(RightGains, RightEnd));
  for (uint32_t I = 0; I < MinSize; I++) {
    if (LeftGains[I].first + RightGains[I].first <= 0.0)
      break;
    // Try to swap the two documents
    NumMovedDataVertices += moveDataVertex(*LeftGains[I].second, LeftBucket,
                                           RightBucket, Signatures, RNG);
    NumMovedDataVertices += moveDataVertex(*RightGains[I].second, LeftBucket,
                                           RightBucket, Signatures, RNG);
  }
  return NumMovedDataVertices;
}

bool BalancedPartitioning::moveDataVertex(Document &Doc, uint32_t LeftBucket,
                                          uint32_t RightBucket,
                                          SignaturesType &Signatures,
                                          std::mt19937 &RNG) const {
  // Sometimes we skip the move. This helps to escape local optima
  if (std::uniform_real_distribution<double>(0.0, 1.0)(RNG) <=
      Config.SkipProbability)
    return false;

  bool FromLeftToRight = (Doc.Bucket == LeftBucket);

  // Update the current bucket
  Doc.Bucket = (FromLeftToRight ? RightBucket : LeftBucket);

  // Update signatures
  for (auto &Kmer : Doc.AdjacentKmers) {
    auto &Signature = Signatures[Kmer];
    Signature.CacheIsInvalid = true;
    if (FromLeftToRight) {
      Signature.LeftCount--;
      Signature.RightCount++;
    } else {
      Signature.LeftCount++;
      Signature.RightCount--;
    }
  }
  return true;
}

void BalancedPartitioning::split(const DocumentRange Docs,
                                 uint32_t StartBucket) const {
  uint32_t NumDocuments = std::distance(Docs.begin(), Docs.end());
  auto DocsMid = Docs.begin() + (NumDocuments + 1) / 2;

  std::nth_element(Docs.begin(), DocsMid, Docs.end(), [](auto &L, auto &R) {
    return L.InputOrder < R.InputOrder;
  });

  for (auto &Doc : llvm::make_range(Docs.begin(), DocsMid))
    Doc.Bucket = StartBucket;
  for (auto &Doc : llvm::make_range(DocsMid, Docs.end()))
    Doc.Bucket = StartBucket + 1;
}

double BalancedPartitioning::moveGain(const Document &Doc, bool FromLeftToRight,
                                      const SignaturesType &Signatures) const {
  double Gain = 0;
  for (auto &Kmer : Doc.AdjacentKmers) {
    auto &Signature = Signatures.find(Kmer)->second;
    Gain += (FromLeftToRight ? Signature.CachedCostLR : Signature.CachedCostRL);
  }
  return Gain;
}

void BalancedPartitioning::prepareSignature(KmerSignature &Signature) const {
  uint32_t L = Signature.LeftCount;
  uint32_t R = Signature.RightCount;
  assert((L > 0 || R > 0) && "incorrect signature");
  double cost = logCost(L, R);
  if (L > 0)
    Signature.CachedCostLR = cost - logCost(L - 1, R + 1);
  if (R > 0)
    Signature.CachedCostRL = cost - logCost(L + 1, R - 1);
}

double BalancedPartitioning::logCost(uint32_t X, uint32_t Y) const {
  return -(X * log2Cached(X + 1) + Y * log2Cached(Y + 1));
}

double
BalancedPartitioning::computeGoal(const SignaturesType &Signatures) const {
  double Sum = 0;
  double Cnt = 0;
  for (auto &[Kmer, Signature] : Signatures) {
    uint32_t L = Signature.LeftCount;
    uint32_t R = Signature.RightCount;
    assert((L > 0 || R > 0) && "incorrect signature");
    // Shifting opt goal to the interval [0..xxx)
    Sum += logCost(L, R) - logCost(L + R, 0);
    Cnt += L + R;
  }
  return Cnt > 0 ? Sum / Cnt : 0;
}

double BalancedPartitioning::log2Cached(uint32_t i) const {
  return (i < LOG_CACHE_SIZE) ? Log2Cache[i] : std::log2(i);
}
