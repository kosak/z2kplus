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
#include <memory>
#include <string>
#include <vector>
#include "catch/catch.hpp"
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/reverse_index/fields.h"
#include "z2kplus/backend/reverse_index/index/consolidated_index.h"
#include "z2kplus/backend/reverse_index/iterators/word/anchored.h"
#include "z2kplus/backend/reverse_index/iterators/zgram/and.h"
#include "z2kplus/backend/reverse_index/iterators/iterator_common.h"
#include "z2kplus/backend/reverse_index/iterators/zgram/metadata/having_reaction.h"
#include "z2kplus/backend/reverse_index/iterators/boundary/near.h"
#include "z2kplus/backend/reverse_index/iterators/zgram/not.h"
#include "z2kplus/backend/reverse_index/iterators/word/pattern.h"
#include "z2kplus/backend/reverse_index/iterators/boundary/word_adaptor.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/test/util/test_util.h"
#include "z2kplus/backend/util/automaton/automaton.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::memory::MappedFile;
using kosak::coding::stringf;
using z2kplus::backend::shared::Zephyrgram;
using z2kplus::backend::reverse_index::index::ConsolidatedIndex;
using z2kplus::backend::reverse_index::FieldMask;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::reverse_index::WordInfo;
using z2kplus::backend::reverse_index::zgramOff_t;
using z2kplus::backend::reverse_index::iterators::boundary::Near;
using z2kplus::backend::reverse_index::iterators::boundary::WordAdaptor;
using z2kplus::backend::reverse_index::iterators::Anchored;
using z2kplus::backend::reverse_index::iterators::And;
using z2kplus::backend::reverse_index::iterators::Not;
using z2kplus::backend::reverse_index::iterators::WordIterator;
using z2kplus::backend::reverse_index::iterators::ZgramIterator;
using z2kplus::backend::reverse_index::iterators::zgram::metadata::HavingReaction;
using z2kplus::backend::reverse_index::iterators::word::Pattern;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::test::util::TestUtil;
using z2kplus::backend::util::automaton::FiniteAutomaton;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::test {

namespace {
bool getPathMaster(std::shared_ptr<PathMaster> *result, const FailFrame &ff);
}  // namespace

// body:kosak
TEST_CASE("reverse_index: body:kosak","[reverse_index]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  ConsolidatedIndex ci;
  FiniteAutomaton dfa;
  if (!getPathMaster(&pm, fr.nest(HERE)) ||
    !TestUtil::trySetupConsolidatedIndex(std::move(pm), &ci, fr.nest(HERE)) ||
    !TestUtil::tryMakeDfa("kosak", &dfa, fr.nest(HERE))) {
    FAIL(fr);
  }

  auto pattern = Pattern::create(std::move(dfa), FieldMask::body);
  auto iterator = WordAdaptor::create(std::move(pattern));
  if (!TestUtil::fourWaySearchTest("kosak", ci, iterator.get(), 5, {4, 50, 63, 70, 71}, fr.nest(HERE))) {
    FAIL(fr);
  }
}

// Searching for "body:^this"
TEST_CASE("reverse_index: body:^this", "[reverse_index]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  ConsolidatedIndex ci;
  FiniteAutomaton thisDfa;
  if (!getPathMaster(&pm, fr.nest(HERE)) ||
    !TestUtil::trySetupConsolidatedIndex(std::move(pm), &ci, fr.nest(HERE)) ||
    !TestUtil::tryMakeDfa("this", &thisDfa, fr.nest(HERE))) {
    FAIL(fr);
  }

  auto thisPattern = Pattern::create(std::move(thisDfa), FieldMask::body);
  auto anchoredThis = Anchored::create(std::move(thisPattern), true, false);
  auto iterator = WordAdaptor::create(std::move(anchoredThis));

  if (!TestUtil::fourWaySearchTest("", ci, iterator.get(), 4, {51}, fr.nest(HERE))) {
    FAIL(fr);
  }
}

// Searching for "^body:kosak$"
TEST_CASE("reverse_index: body:^FAIL$", "[reverse_index]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  ConsolidatedIndex ci;
  FiniteAutomaton failDfa;
  if (!getPathMaster(&pm, fr.nest(HERE)) ||
    !TestUtil::trySetupConsolidatedIndex(std::move(pm), &ci, fr.nest(HERE)) ||
    !TestUtil::tryMakeDfa("FAIL", &failDfa, fr.nest(HERE))) {
    FAIL(fr);
  }

  auto fail = Pattern::create(std::move(failDfa), FieldMask::body);
  auto anchoredKosak = Anchored::create(std::move(fail), true, true);
  auto iterator = WordAdaptor::create(std::move(anchoredKosak));

  if (!TestUtil::fourWaySearchTest("", ci, iterator.get(), 4, {52}, fr.nest(HERE))) {
    FAIL(fr);
  }
}

// Testing "instance:^*$", i.e. instances composed of exactly one word
TEST_CASE("reverse_index: instance:^*$", "[reverse_index]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  ConsolidatedIndex ci;
  FiniteAutomaton starDfa;
  if (!getPathMaster(&pm, fr.nest(HERE)) ||
    !TestUtil::trySetupConsolidatedIndex(pm, &ci, fr.nest(HERE)) ||
    !TestUtil::tryMakeDfa("*", &starDfa, fr.nest(HERE))) {
    FAIL(fr);
  }

  auto star = Pattern::create(std::move(starDfa), FieldMask::instance);
  auto anchoredStar = Anchored::create(std::move(star), true, true);
  auto iterator = WordAdaptor::create(std::move(anchoredStar));
  if (!TestUtil::fourWaySearchTest("", ci, iterator.get(), 4,
      {10, 20, 21, 22, 41, 42, 50, 51, 60, 61, 62, 63, 70, 72}, fr.nest(HERE))) {
    FAIL(fr);
  }
}

// Testing "not all:kosak"
TEST_CASE("reverse_index: not all:kosak", "[reverse_index]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  ConsolidatedIndex ci;
  FiniteAutomaton kosakDfa;
  if (!getPathMaster(&pm, fr.nest(HERE)) ||
    !TestUtil::trySetupConsolidatedIndex(std::move(pm), &ci, fr.nest(HERE)) ||
    !TestUtil::tryMakeDfa("kosak", &kosakDfa, fr.nest(HERE))) {
    FAIL(fr);
  }

  auto kosak = Pattern::create(std::move(kosakDfa), FieldMask::all);
  auto adapted = WordAdaptor::create(std::move(kosak));
  auto iterator = Not::create(std::move(adapted));

  if (!TestUtil::fourWaySearchTest("", ci, iterator.get(), 4, {2, 21, 40, 41, 42, 52}, fr.nest(HERE))) {
    FAIL(fr);
  }
}

// sender:kosak and not signature:kosak
TEST_CASE("reverse_index: sender:kosak and not signature:kosak", "[reverse_index]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  // We cannot, alas, share these DFAs, because Pattern wants to own its DFA.
  ConsolidatedIndex ci;
  FiniteAutomaton kosak1Dfa;
  FiniteAutomaton kosak2Dfa;
  if (!getPathMaster(&pm, fr.nest(HERE)) ||
    !TestUtil::trySetupConsolidatedIndex(std::move(pm), &ci, fr.nest(HERE)) ||
    !TestUtil::tryMakeDfa("kosak", &kosak1Dfa, fr.nest(HERE)) ||
    !TestUtil::tryMakeDfa("kosak", &kosak2Dfa, fr.nest(HERE))) {
    FAIL(fr);
  }

  auto kosak1 = Pattern::create(std::move(kosak1Dfa), FieldMask::sender);
  auto kosak2 = Pattern::create(std::move(kosak2Dfa), FieldMask::signature);
  auto adapted1 = WordAdaptor::create(std::move(kosak1));
  auto adapted2 = WordAdaptor::create(std::move(kosak2));
  auto notAdapted2 = Not::create(std::move(adapted2));
  std::vector<std::unique_ptr<ZgramIterator>> children;
  children.push_back(std::move(adapted1));
  children.push_back(std::move(notAdapted2));
  auto iterator = And::create(std::move(children));

  if (!TestUtil::fourWaySearchTest("", ci, iterator.get(), 4, {30}, fr.nest(HERE))) {
    FAIL(fr);
  }
}

// "corey kosak"
TEST_CASE("reverse_index: near(1 the the zamboni)", "[reverse_index]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  ConsolidatedIndex ci;
  FiniteAutomaton the1Dfa;
  FiniteAutomaton the2Dfa;
  FiniteAutomaton zamboniDfa;
  if (!getPathMaster(&pm, fr.nest(HERE)) ||
    !TestUtil::trySetupConsolidatedIndex(pm, &ci, fr.nest(HERE)) ||
    !TestUtil::tryMakeDfa("the", &the1Dfa, fr.nest(HERE)) ||
    !TestUtil::tryMakeDfa("the", &the2Dfa, fr.nest(HERE)) ||
    !TestUtil::tryMakeDfa("zamboni", &zamboniDfa, fr.nest(HERE))) {
    FAIL(fr);
  }
  auto the1 = Pattern::create(std::move(the1Dfa), FieldMask::body);
  auto the2 = Pattern::create(std::move(the2Dfa), FieldMask::body);
  auto zanboni = Pattern::create(std::move(zamboniDfa), FieldMask::body);
  std::vector<std::unique_ptr<WordIterator>> children;
  children.push_back(std::move(the1));
  children.push_back(std::move(the2));
  children.push_back(std::move(zanboni));
  auto iterator = Near::create(1, std::move(children));
  if (!TestUtil::fourWaySearchTest("", ci, iterator.get(), 4, {60}, fr.nest(HERE))) {
    FAIL(fr);
  }
}

namespace {
bool testNear(const ConsolidatedIndex &ci, size_t margin, uint64 startZgramIdRaw,
  std::initializer_list<uint64> expectedRaw, const FailFrame &ff) {
  FiniteAutomaton youDfa;
  FiniteAutomaton jealousDfa;
  if (!TestUtil::tryMakeDfa("you", &youDfa, ff.nest(HERE)) ||
    !TestUtil::tryMakeDfa("jealous", &jealousDfa, ff.nest(HERE))) {
    return false;
  }
  auto you = Pattern::create(std::move(youDfa), FieldMask::body);
  auto jealous = Pattern::create(std::move(jealousDfa), FieldMask::body);
  std::vector<std::unique_ptr<WordIterator>> children;
  children.push_back(std::move(you));
  children.push_back(std::move(jealous));
  auto iterator = Near::create(margin, std::move(children));

  auto callerInfo = stringf("margin=%o", margin);
  return TestUtil::fourWaySearchTest(callerInfo, ci, iterator.get(), startZgramIdRaw, expectedRaw,
      ff.nest(HERE));
}
}  // namespace

// near(3, you, jealous)
TEST_CASE("reverse_index: near(N you jealous)", "[reverse_index]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;

  std::array<std::initializer_list<uint64>, 4> expecteds{
      std::initializer_list<uint64>{},
      std::initializer_list<uint64>{},
      std::initializer_list<uint64>{23},
      std::initializer_list<uint64>{23}
  };

  ConsolidatedIndex ci;
  if (!getPathMaster(&pm, fr.nest(HERE)) ||
    !TestUtil::trySetupConsolidatedIndex(pm, &ci, fr.nest(HERE))) {
    FAIL(fr);
  }
  for (size_t i = 0; i < expecteds.size(); ++i) {
    if (!testNear(ci, i + 1, 4, expecteds[i], fr.nest(HERE))) {
      FAIL(fr);
    }
  }
}

namespace {
bool makeThisManyAdjacentThes(size_t numThes, FieldMask fieldMask,
    std::unique_ptr<ZgramIterator> *result, const FailFrame &ff) {
  std::vector<std::unique_ptr<WordIterator>> children;
  for (size_t i = 0; i < numThes; ++i) {
    FiniteAutomaton dfa;
    if (!TestUtil::tryMakeDfa("the", &dfa, ff.nest(HERE))) {
      return false;
    }
    children.push_back(Pattern::create(std::move(dfa), fieldMask));
  }
  *result = Near::create(1, std::move(children));
  return true;
}
}  // namespace

// Searching for "the", "the the", etc, up to six "thes"
TEST_CASE("reverse_index: various 'the's", "[reverse_index]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  ConsolidatedIndex ci;
  if (!getPathMaster(&pm, fr.nest(HERE)) ||
    !TestUtil::trySetupConsolidatedIndex(pm, &ci, fr.nest(HERE))) {
    FAIL(fr);
  }

  std::array<std::initializer_list<uint64>, 6> expecteds{
      std::initializer_list<uint64>{0, 10, 11, 12, 20, 30, 41, 42, 50, 60, 61},
      std::initializer_list<uint64>{60, 61},
      std::initializer_list<uint64>{61},
      std::initializer_list<uint64>{61},
      std::initializer_list<uint64>{61},
      std::initializer_list<uint64>{},
  };

  for (size_t i = 0; i < expecteds.size(); ++i) {
    debug(R"(Making %o adjacent "the"'s")", i + 1);
    std::unique_ptr<ZgramIterator> iterator;
    if (!makeThisManyAdjacentThes(i + 1, FieldMask::body, &iterator, fr.nest(HERE))) {
      FAIL(fr);
    }
    auto callerInfo = stringf("Num thes=%o", i + 1);
    if (!TestUtil::fourWaySearchTest(callerInfo, ci, iterator.get(), 5, expecteds[i], fr.nest(HERE))) {
      FAIL(fr);
    }
  }
}

// Searching for havingreaction("👎")
TEST_CASE("reverse_index: havingreaction('👎')", "[reverse_index]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  ConsolidatedIndex ci;
  if (!getPathMaster(&pm, fr.nest(HERE)) ||
    !TestUtil::trySetupConsolidatedIndex(pm, &ci, fr.nest(HERE))) {
    FAIL(fr);
  }
  auto iterator = HavingReaction::create("👎");
  if (!TestUtil::fourWaySearchTest("", ci, iterator.get(), 4, {1, 42}, fr.nest(HERE))) {
    FAIL(fr);
  }
}

static bool searchForPattern(const std::string_view &word, uint64 startIdRaw,
    std::initializer_list<uint64> expected, const FailFrame &ff) {
  std::shared_ptr<PathMaster> pm;
  ConsolidatedIndex ci;
  FiniteAutomaton dfa;
  if (!getPathMaster(&pm, ff.nest(HERE)) ||
    !TestUtil::trySetupConsolidatedIndex(pm, &ci, ff.nest(HERE)) ||
    !TestUtil::tryMakeDfa(word, &dfa, ff.nest(HERE))) {
    return false;
  }
  debug("The DFA looks like this: %o", dfa);

  auto pattern = Pattern::create(std::move(dfa), FieldMask::body);
  auto iterator = WordAdaptor::create(std::move(pattern));
  return TestUtil::fourWaySearchTest("", ci, iterator.get(), startIdRaw, expected, ff.nest(HERE));
}

TEST_CASE("reverse_index: ❤","[reverse_index]") {
  FailRoot fr;
  if (!searchForPattern("❤", 2, {12}, fr.nest(HERE))) {
    FAIL(fr);
  }
}

// Misspelled: no a
TEST_CASE("reverse_index: cinnbon","[reverse_index]") {
  FailRoot fr;
  if (!searchForPattern("cinnbon", 2, {}, fr.nest(HERE))) {
    FAIL(fr);
  }
}

// Correctly spelled
TEST_CASE("reverse_index: cinnabon","[reverse_index]") {
  FailRoot fr;
  if (!searchForPattern("cinnabon", 3, {10, 11, 12}, fr.nest(HERE))) {
    FAIL(fr);
  }
}

TEST_CASE("reverse_index: cinn?bon","[reverse_index]") {
  FailRoot fr;
  if (!searchForPattern("cinn?bon", 3, {10, 11, 12}, fr.nest(HERE))) {
    FAIL(fr);
  }
}

TEST_CASE("reverse_index: c*n","[reverse_index]") {
  FailRoot fr;
  if (!searchForPattern("c*n", 3, {10, 11, 12}, fr.nest(HERE))) {
    FAIL(fr);
  }
}

TEST_CASE("reverse_index: *c*b*n*","[reverse_index]") {
  FailRoot fr;
  if (!searchForPattern("*c*b*n*", 3, {10, 11, 12, 13}, fr.nest(HERE))) {
    FAIL(fr);
  }
}

namespace {
bool getPathMaster(std::shared_ptr<PathMaster> *result, const FailFrame &ff) {
  return TestUtil::tryGetPathMaster("reverse_index", result, ff.nest(HERE));
}
}  // namespace
}  // namespace z2kplus::backend::test
