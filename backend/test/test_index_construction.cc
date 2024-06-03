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

#include <cstdint>
#include <list>
#include <string>
#include "catch/catch.hpp"
#include "kosak/coding/coding.h"
#include "kosak/coding/containers/slice.h"
#include "kosak/coding/memory/mapped_file.h"
#include "kosak/coding/text/conversions.h"
#include "kosak/coding/unix.h"
#include "z2kplus/backend/factories/log_parser.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/reverse_index/builder/index_builder.h"
#include "z2kplus/backend/reverse_index/index/dynamic_index.h"
#include "z2kplus/backend/reverse_index/index/frozen_index.h"
#include "z2kplus/backend/reverse_index/trie/dynamic_trie.h"
#include "z2kplus/backend/reverse_index/types.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/shared/protocol/misc.h"
#include "z2kplus/backend/util/misc.h"
#include "z2kplus/backend/test/util/test_util.h"

using kosak::coding::containers::asSlice;
using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::memory::MappedFile;
using kosak::coding::text::ReusableString32;
using z2kplus::backend::factories::LogParser;
using z2kplus::backend::files::CompressedFileKey;
using z2kplus::backend::files::LogLocation;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::shared::LogRecord;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::shared::Zephyrgram;
using z2kplus::backend::reverse_index::builder::IndexBuilder;
using z2kplus::backend::reverse_index::index::ConsolidatedIndex;
using z2kplus::backend::reverse_index::index::DynamicIndex;
using z2kplus::backend::reverse_index::index::FrozenIndex;
using z2kplus::backend::reverse_index::FieldMask;
using z2kplus::backend::reverse_index::WordInfo;
using z2kplus::backend::reverse_index::iterators::WordIterator;
using z2kplus::backend::reverse_index::iterators::ZgramIterator;
using z2kplus::backend::reverse_index::trie::DynamicTrie;
using z2kplus::backend::reverse_index::trie::FrozenTrie;
using z2kplus::backend::reverse_index::wordOff_t;
using z2kplus::backend::test::util::TestUtil;
using z2kplus::backend::util::automaton::FiniteAutomaton;

namespace nsunix = kosak::coding::nsunix;
namespace indexBuilder = z2kplus::backend::reverse_index::builder;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::test {

namespace {
bool tryGetPathMaster(std::shared_ptr<PathMaster> *result, const FailFrame &ff);

auto simpleKey0 = FileKey::createUnsafe(2000, 1, 1, 0, true);
const char simpleText0[] = "" // to help CLion align the next line
  R"([["z",[[0],946703313,"kosak","Corey Kosak",true,["test","Hello this is kosak","d"]]]])" "\n";

auto simpleKey1 = FileKey::createUnsafe(2000, 1, 1, 1, true);
const char simpleText1[] = "" // to help CLion align the next line
  R"([["z",[[1],946703314,"kosh","Kosh",true,["test","You are not ready","d"]]]])" "\n"
  // body revision of zgram 0
  R"([["m",[["zgrev",[[0],["test","I am Kosh","d"]]]]]])" "\n";
}  // namespace

TEST_CASE("index_construction: Probe Dynamic Trie", "[index_construction]") {
  DynamicTrie trie;
  auto data = std::experimental::make_array(wordOff_t(1), wordOff_t(2), wordOff_t(3));
  auto goodProbes = std::experimental::make_array("kosak", "kosakowski", "kosa", "kosh", "Hello");
  auto badProbes = std::experimental::make_array("", "kos", "kosako");
  ReusableString32 rs32;
  for (const char *probe : goodProbes) {
    INFO("inserting " << probe);
    trie.insert(TestUtil::friendlyReset(&rs32, probe), data.begin(), data.size());
  }
  debug("trie is %o", trie);
  std::pair<const wordOff_t*, const wordOff_t*> result;
  for (const char *probe : goodProbes) {
    INFO("Searching for good probe " << probe);
    CHECK(true == trie.tryFind(TestUtil::friendlyReset(&rs32, probe), &result));
  }
  for (const char *probe : badProbes) {
    INFO("Searching for bad probe " << probe);
    INFO(probe);
    CHECK(false == trie.tryFind(TestUtil::friendlyReset(&rs32, probe), &result));
  }
}

TEST_CASE("index_construction: Build Dynamic Index", "[index_construction]") {
  FailRoot fr;
  FrozenIndex empty;
  DynamicIndex di;
  std::vector<LogParser::logRecordAndLocation_t> items;
  if (!LogParser::tryParseLogRecords(simpleText0, simpleKey0, &items, fr.nest(HERE)) ||
      !LogParser::tryParseLogRecords(simpleText1, simpleKey1, &items, fr.nest(HERE)) ||
      !di.tryAddLogRecords(empty, items, fr.nest(HERE))) {
    FAIL(fr);
  }
  debug("Index is %o", di);
  std::pair<const wordOff_t*, const wordOff_t*> result;
  ReusableString32 rs32;
  REQUIRE(true == di.trie().tryFind(TestUtil::friendlyReset(&rs32, "Kosh"), &result));
  REQUIRE(1 == result.second - result.first);
  // In this test Kosh appears at offset 9. In the next test, after the body revision is processed,
  // Kosh will move to position 10.
  REQUIRE(9 == result.first->raw());
}

TEST_CASE("index_construction: Build Frozen Index", "[index_construction]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  MappedFile<FrozenIndex> mf;
  if (!tryGetPathMaster(&pm, fr.nest(HERE)) ||
    !TestUtil::tryPopulateFile(*pm, simpleKey0, simpleText0, fr.nest(HERE)) ||
    !TestUtil::tryPopulateFile(*pm, simpleKey1, simpleText1, fr.nest(HERE)) ||
    !IndexBuilder::tryBuild(*pm, {}, {}, {}, {}, fr.nest(HERE)) ||
    !pm->tryPublishBuild(fr.nest(HERE)) ||
    !mf.tryMap(pm->getIndexPath(), false, fr.nest(HERE))) {
    FAIL(fr);
  }
  const auto *index = mf.get();
  std::pair<const wordOff_t*, const wordOff_t*> result;
  ReusableString32 rs32;
  REQUIRE(true == index->trie().tryFind(TestUtil::friendlyReset(&rs32, "Kosh"), &result));
  REQUIRE(2 == result.second - result.first);
  // In this test Kosh appears at offset 9. In the next test, after the body revision is processed,
  // there will be two Koshes, at 6 and 8.
  REQUIRE(6 == result.first[0].raw());
  REQUIRE(8 == result.first[1].raw());
}

TEST_CASE("index_construction: Probe Frozen Index", "[index_construction]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  MappedFile<FrozenIndex> mf;
  DateAndPartKey lastKey;
  if (!tryGetPathMaster(&pm, fr.nest(HERE)) ||
    !TestUtil::tryPopulateFile(*pm, simpleKey0, simpleText0, fr.nest(HERE)) ||
    !TestUtil::tryPopulateFile(*pm, simpleKey1, simpleText1, fr.nest(HERE)) ||
    !IndexBuilder::tryBuild(*pm, {}, {}, {}, {}, fr.nest(HERE)) ||
    !pm->tryPublishBuild(fr.nest(HERE)) ||
    !mf.tryMap(pm->getIndexPath(), false, fr.nest(HERE))) {
    FAIL(fr);
  }

  auto goodProbes = std::experimental::make_array("kosak", "Kosak", "Corey", "kosh", "Kosh", "not");
  auto badProbes = std::experimental::make_array("", "k", "kos", "kosa", "is");

  const auto *index = mf.get();
  std::pair<const wordOff_t*, const wordOff_t*> result;
  INFO(index->trie());
  ReusableString32 rs32;
  for (const char *probe : goodProbes) {
    INFO(probe);
    CHECK(true == index->trie().tryFind(TestUtil::friendlyReset(&rs32, probe), &result));
  }
  for (const char *probe : badProbes) {
    INFO(probe);
    CHECK(false == index->trie().tryFind(TestUtil::friendlyReset(&rs32, probe), &result));
  }
}

namespace {
bool tryGetPathMaster(std::shared_ptr<PathMaster> *result, const FailFrame &ff) {
  return TestUtil::tryGetPathMaster("index_construction", result, ff.nest(HERE));
}
}  // namespace
}  // namespace z2kplus::backend::test
