//===- BalancedPartitioningTest.cpp - BalancedPartitioning tests ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/BalancedPartitioning.h"
#include "llvm/Testing/Support/SupportHelpers.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {
using ::testing::ElementsAre;

TEST(BalancedPartitioningTest, Basic) {
  DenseSet<uint32_t> Kmers1 = {1, 2};
  DenseSet<uint32_t> Kmers2 = {3, 4};
  DenseSet<uint32_t> Kmers3 = {4};
  std::vector<Document> Documents = {
      Document(1, Kmers1), //
      Document(3, Kmers2), //
      Document(2, Kmers1), //
      Document(4, Kmers2), //
      Document(5, Kmers3), //
  };

  BalancedPartitioningConfig Config;
  BalancedPartitioning Bp(Config);
  Bp.run(Documents);

  std::vector<uint64_t> OrderedIds;
  for (auto &Doc : Documents)
    OrderedIds.push_back(Doc.Id);
  EXPECT_THAT(OrderedIds, ElementsAre(1, 2, 3, 4, 5));
}

} // namespace
