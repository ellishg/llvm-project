//===- BalancedPartitioning.h ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// TODO
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_BALANCED_PARTITIONING_H
#define LLVM_SUPPORT_BALANCED_PARTITIONING_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/ThreadPool.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/ProfileData/InstrProf.h"

#include <random>
#include <string>
#include <vector>

namespace llvm {

class Document;
class KmerSignature;

using KmerT = uint32_t;
using DocumentIDT = uint64_t;

///
class Document {
  friend class BalancedPartitioning;

public:
  Document(DocumentIDT Id, DenseSet<KmerT> &AdjacentKmers)
      : Id(Id), AdjacentKmers(AdjacentKmers) {}

  DocumentIDT Id;

  static std::vector<Document>
  fromTraces(const SmallVectorImpl<std::vector<uint64_t>> &Traces);

protected:
  ///
  DenseSet<KmerT> AdjacentKmers;
  /// Document bucket assigned by balanced partitioning.
  std::optional<uint32_t> Bucket;
  /// The input order of the documents.
  uint64_t InputOrder = 0;

public:
  void dump(raw_ostream &OS, InstrProfSymtab *Symtab = nullptr);
};

/// Algorithm parameters; default values are tuned on real-world binaries.
struct BalancedPartitioningConfig {
  /// The depth of the recursive bisection.
  uint32_t SplitDepth = 16;
  /// The maximum number of bp iterations per split.
  uint32_t IterationsPerSplit = 40;
  /// The probability for a vertex to skip a move from its current bucket to
  /// another bucket; it often helps to escape from a local optima.
  double SkipProbability = 0.1;
  /// Recursive subtasks up to the given depth are added to the queue and
  /// distributed among threads by ThreadPool; all subsequent calls are executed
  /// on the same thread.
  uint32_t TaskSplitDepth = 9;
};

/// Recursive balanced graph partitioning algorithm.
///
/// The algorithm is used to find an ordering of Documents while optimizing
/// a specified objective. The algorithm uses recursive bisection; it starts
/// with a collection of unordered documents and tries to split them into
/// two sets (buckets) of equal cardinality. Each bisection step is comprised of
/// iterations that greedily swap the documents between the two buckets while
/// there is an improvement of the objective. Once the process converges, the
/// problem is divided into two sub-problems of half the size, which are
/// recursively applied for the two buckets. The final ordering of the documents
/// is obtained by concatenating the two (recursively computed) orderings.
///
/// In order to speed up the computation, we limit the depth of the recursive
/// tree by a specified constant (SplitDepth) and apply at most a constant
/// number of greedy iterations per split (IterationsPerSplit). The worst-case
/// time complexity of the implementation is bounded by O(M*log^2 N), where
/// N is the number of documents and M is the number of document-kmer edges;
/// (assuming that any collection of D documents contains O(D) k-mers). Notice
/// that the two different recursive sub-problems are independent and thus can
/// be efficiently processed in parallel.
class BalancedPartitioning {
  using SignaturesType = DenseMap<KmerT, KmerSignature>;

  using DocumentRange = iterator_range<std::vector<Document>::iterator>;

private:
  BalancedPartitioning(const BalancedPartitioning &) = delete;
  BalancedPartitioning &operator=(const BalancedPartitioning &) = delete;

public:
  BalancedPartitioning(const BalancedPartitioningConfig &Config);

  /// Run recursive graph partitioning that optimizes a given objective.
  void run(std::vector<Document> &Documents) const;

private:
  /// Run a recursive bisection of a given list of documents where
  ///  - 'RecDepth' is the current depth of recursion
  ///  - 'RootBucket' is the initial bucket of the dataVertices
  ///  - the assigned buckets are the range [Offset, Offset + NumDocuments)
  void bisect(const DocumentRange Docs, uint32_t RecDepth, uint32_t RootBucket,
              uint32_t Offset) const;

  /// Run bisection iterations.
  /// Returns true iff a progress has been made.
  bool runIterations(const DocumentRange Docs, uint32_t RecDepth,
                     uint32_t LeftBucket, uint32_t RightBucket,
                     std::mt19937 &RNG) const;

  /// Run a bisection iteration to improve the optimization goal.
  /// Returns the total number of moved documents.
  uint32_t runIteration(const DocumentRange Docs, uint32_t LeftBucket,
                        uint32_t RightBucket, SignaturesType &Signatures,
                        std::mt19937 &RNG) const;

  /// Try to move a document from one bucket to another.
  /// Return true iff the document is moved.
  bool moveDataVertex(Document &document, uint32_t LeftBucket,
                      uint32_t RightBucket, SignaturesType &Signatures,
                      std::mt19937 &RNG) const;

  /// Initialize k-mer signatures.
  void initializeSignatures(SignaturesType &Signatures,
                            const DocumentRange Docs,
                            uint32_t LeftBucket) const;

  /// Split all the documents into 2 buckets, StartBucket and StartBucket + 1.
  /// The method is used for an initial assignment before a bisection step.
  void split(const DocumentRange Docs, uint32_t StartBucket) const;

  /// Initialize k-mer signature before a bisection iteration.
  void prepareSignature(KmerSignature &Signature) const;

  /// Compute the move gain for uniform log-gap cost:
  /// cost = x * log(U / (x+1)) + y * log(U / (y+1)) =
  ///      = x * log(U) + y * log(U) - (x * log(x+1) + y * log(y+1)) =
  ///      = U * log(U) - (x * log(x+1) + y * log(y+1))
  /// The first term is constant; the second is 'logCost'.
  double moveGain(const Document &Doc, bool FromLeftToRight,
                  const SignaturesType &Signatures) const;

  /// The cost of the uniform log-gap cost, assuming a k-mer has X
  /// documents in the left bucket and Y documents in the right one.
  double logCost(uint32_t X, uint32_t Y) const;

  /// Compute an average optimization goal for a given k-mer signature:
  /// - to represent an integer k, one needs log_2(k) bits;
  /// - to represent n integers in the range [0..U) (using the diff encoding),
  ///   one needs log_2(U/n) per number, since an average diff is U/n.
  /// Hence, n integers in the range [0..U) require (2 + log(U/n))*n bits, where
  /// two additional bits is a constant overhead.
  double computeGoal(const SignaturesType &Signatures) const;

  double log2Cached(uint32_t i) const;

private:
  const BalancedPartitioningConfig &Config;

  /// A single thread pool that is used to run parallel tasks.
  std::unique_ptr<ThreadPool> ThreadPoolPtr;

  /// Precomputed values of log2(x). Table size is small enough to fit in cache.
  static constexpr uint32_t LOG_CACHE_SIZE = 16384;
  double Log2Cache[LOG_CACHE_SIZE];
};

/// Signature of a Kmer utilized in a bisection step, that is, the number of
/// incident documents in the two buckets.
class KmerSignature {
public:
  KmerSignature(const KmerSignature &) = delete;
  KmerSignature(KmerSignature &&) = default;
  KmerSignature &operator=(const KmerSignature &) = delete;
  KmerSignature &operator=(KmerSignature &&) = default;

  explicit KmerSignature(uint32_t LeftCount = 0, uint32_t RightCount = 0)
      : LeftCount(LeftCount), RightCount(RightCount) {}

public:
  /// The number of documents in the left bucket.
  uint32_t LeftCount;
  /// The number of documents in the right bucket.
  uint32_t RightCount;
  /// Cached cost of moving a document from left to right bucket.
  double CachedCostLR = 0;
  /// Cached cost of moving a document from right to left bucket.
  double CachedCostRL = 0;
  /// Whether the cached costs must be recomputed.
  bool CacheIsInvalid = true;
};

} // end namespace llvm

#endif // LLVM_SUPPORT_BALANCED_PARTITIONING_H
