// Copyright 2023 The Z2K Plus+ Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <list>
#include <string>
#include "catch/catch.hpp"
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/map_utils.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/reverse_index/index/consolidated_index.h"
#include "z2kplus/backend/util/frozen/frozen_string_pool.h"
#include "z2kplus/backend/test/util/test_util.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::maputils::tryFind;
using kosak::coding::toString;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::shared::LogRecord;
using z2kplus::backend::shared::MetadataRecord;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::shared::Zephyrgram;
using z2kplus::backend::reverse_index::index::ConsolidatedIndex;
using z2kplus::backend::reverse_index::FieldMask;
using z2kplus::backend::reverse_index::WordInfo;
using z2kplus::backend::reverse_index::iterators::WordIterator;
using z2kplus::backend::reverse_index::iterators::ZgramIterator;
using z2kplus::backend::reverse_index::metadata::DynamicMetadata;
using z2kplus::backend::reverse_index::metadata::FrozenMetadata;
using z2kplus::backend::test::util::TestUtil;
using z2kplus::backend::util::automaton::FiniteAutomaton;
using z2kplus::backend::util::frozen::frozenStringRef_t;

namespace zgMetadata = z2kplus::backend::shared::zgMetadata;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::test {

namespace {
bool getPathMaster(std::shared_ptr<PathMaster> *result, const FailFrame &ff);
}  // namespace

// body:kosak
TEST_CASE("metadata: perZgram","[metadata]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  ConsolidatedIndex ci;
  if (!getPathMaster(&pm, fr.nest(HERE)) ||
    !TestUtil::trySetupConsolidatedIndex(pm, &ci, fr.nest(HERE))) {
    FAIL(fr);
  }

  {
    std::vector<MetadataRecord> mdrs;
    ci.getMetadataFor(ZgramId(30), &mdrs);
    REQUIRE(mdrs.size() == 3);
    auto ip = mdrs.begin();
    const auto *item0 = std::get_if<zgMetadata::Reaction>(&ip++->payload());
    const auto *item1 = std::get_if<zgMetadata::Reaction>(&ip++->payload());
    const auto *item2 = std::get_if<zgMetadata::Reaction>(&ip++->payload());
    CHECK(item0->zgramId().raw() == 30);
    CHECK(item1->zgramId().raw() == 30);
    CHECK(item2->zgramId().raw() == 30);

    CHECK(item0->creator() == "simon");
    CHECK(item1->creator() == "kosak");
    CHECK(item2->creator() == "wilhelm");

    CHECK(item0->reaction() == "☢");
    CHECK(item1->reaction() == "👍");
    CHECK(item2->reaction() == "👍");
  }

  {
    std::vector<MetadataRecord> mdrs;
    ci.getMetadataFor(ZgramId(41), &mdrs);
    REQUIRE(mdrs.size() == 2);
    auto ip = mdrs.begin();
    const auto *item0 = std::get_if<zgMetadata::Reaction>(&ip++->payload());
    const auto *item1 = std::get_if<zgMetadata::Reaction>(&ip++->payload());
    CHECK(item0->zgramId().raw() == 41);
    CHECK(item1->zgramId().raw() == 41);

    CHECK(item0->creator() == "kosak");
    CHECK(item1->creator() == "spock");

    CHECK(item0->reaction() == "👍");
    CHECK(item1->reaction() == "👍");
  }

  {
    std::vector<MetadataRecord> mdrs;
    ci.getMetadataFor(ZgramId(42), &mdrs);
    REQUIRE(mdrs.size() == 2);

    size_t numRecords = 0;
    for (const auto &mdr : mdrs) {
      const auto *rx = std::get_if<zgMetadata::Reaction>(&mdr.payload());
      if (rx == nullptr) {
        continue;
      }
      CHECK(rx->zgramId().raw() == 42);
      CHECK(rx->creator() == "spock");
      CHECK(rx->reaction() == "👎");
      ++numRecords;
    }
    CHECK(numRecords == 1);
  }
}

TEST_CASE("metadata: perUserid","[metadata]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  ConsolidatedIndex ci;
  if (!getPathMaster(&pm, fr.nest(HERE)) ||
    !TestUtil::trySetupConsolidatedIndex(pm, &ci, fr.nest(HERE))) {
    FAIL(fr);
  }

  auto userIds = std::experimental::make_array("kosak", "simon");
  auto expectedValues = std::experimental::make_array("❦,❧,💕,💞,🙆,🙅,😂", "☢");
  static_assert(userIds.size() == expectedValues.size());
  for (size_t i = 0; i < userIds.size(); ++i) {
    std::string_view expected(expectedValues[i]);
    auto actual = ci.getZmojis(userIds[i]);
    INFO("Getting zmojis for " << userIds[i]);
    CHECK(expected == actual);
  }
}

namespace {
void testReactionsIndex(const ConsolidatedIndex &ci, const char *reaction,
    const std::vector<uint64_t> &rawIds) {
  std::map<ZgramId, int64_t> netCounts;

  frozenStringRef_t fsr;
  const FrozenMetadata::reactionCounts_t::mapped_type *fInner;
#ifndef NDEBUG
  debug("This is the frozen index");
  for (const auto &entry : ci.frozenIndex().metadata().reactions()) {
    debug("RX - %o: %o", entry.first, entry.second);
  }
  for (const auto &entry : ci.frozenIndex().metadata().reactionCounts()) {
    debug("rc - %o(%o): %o", entry.first, ci.frozenIndex().stringPool().toStringView(entry.first), entry.second);
  }
  for (size_t i = 0; i != ci.frozenIndex().stringPool().size(); ++i) {
    frozenStringRef_t qqq(i);
    debug("%o: %o", i, ci.frozenIndex().stringPool().toStringView(qqq));
  }
#endif

  if (ci.frozenIndex().stringPool().tryFind(reaction, &fsr) &&
      ci.frozenIndex().metadata().reactionCounts().tryFind(fsr, &fInner)) {
    for (const auto &[zgId, count] : *fInner) {
      netCounts[zgId] += count;
    }
  }

  const DynamicMetadata::reactionCounts_t::mapped_type *dInner;
  if (tryFind(ci.dynamicIndex().metadata().reactionCounts(), reaction, &dInner)) {
    for (const auto &[zgId, count] : *dInner) {
      netCounts[zgId] += count;
    }
  }

  std::vector<ZgramId> nonZeroes;
  for (const auto &[zg, count] : netCounts) {
    if (count != 0) {
      nonZeroes.push_back(zg);
    }
  }

  std::vector<ZgramId> expected;
  expected.reserve(rawIds.size());
  for (const auto &rawId : rawIds) {
    expected.emplace_back(rawId);
  }
  INFO("processing reaction " << reaction);
  CHECK(expected == nonZeroes);
}
}  // namespace

TEST_CASE("metadata: reactionsIndex","[metadata]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  ConsolidatedIndex ci;
  if (!getPathMaster(&pm, fr.nest(HERE)) ||
    !TestUtil::trySetupConsolidatedIndex(pm, &ci, fr.nest(HERE))) {
    FAIL(fr);
  }

  auto reactions = std::experimental::make_array("👍", "👎", "☢", "k-wrong");
  std::vector<std::vector<uint64_t>> expectedZgramIds = {
      { 0, 30, 41 },  // These zgrams have "👍" reactions.
      { 1, 42 },  // These zgrams have "👎" reactions.
      { 12, 30 },  // These zgrams have "☢" reactions.
      { 13, 14, 15, 50}  // These zgrams have "k-wrong" reactions.
  };
  REQUIRE(reactions.size() == expectedZgramIds.size());
  for (size_t i = 0; i < reactions.size(); ++i) {
    testReactionsIndex(ci, reactions[i], expectedZgramIds[i]);
  }
}

TEST_CASE("metadata: refersTo","[metadata]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  ConsolidatedIndex ci;
  if (!getPathMaster(&pm, fr.nest(HERE)) ||
      !TestUtil::trySetupConsolidatedIndex(pm, &ci, fr.nest(HERE))) {
    FAIL(fr);
  }

  std::vector<zgMetadata::ZgramRefersTo> actual;
  ci.getRefersToFor(ZgramId(42), &actual);

  REQUIRE(actual.size() == 1);
  const auto &a0 = actual[0];
  CHECK(a0.value());
  CHECK(a0.zgramId().raw() == 42);
  CHECK(a0.refersTo().raw() == 41);
}

namespace {
bool getPathMaster(std::shared_ptr<PathMaster> *result, const FailFrame &ff) {
  return TestUtil::tryGetPathMaster("metadata", result, ff.nest(HERE));
}
}  // namespace
}  // namespace z2kplus::backend::test
