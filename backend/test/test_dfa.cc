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
#include <vector>
#include "catch/catch.hpp"
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/text/conversions.h"
#include "z2kplus/backend/files/keys.h"
#include "z2kplus/backend/queryparsing/util.h"
#include "z2kplus/backend/util/automaton/automaton.h"
#include "z2kplus/backend/util/misc.h"
#include "z2kplus/backend/test/util/test_util.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::text::ReusableString32;
using kosak::coding::text::tryConvertUtf8ToUtf32;
using z2kplus::backend::files::LogLocation;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::queryparsing::WordSplitter;
using z2kplus::backend::reverse_index::FieldMask;
using z2kplus::backend::reverse_index::WordInfo;
using z2kplus::backend::util::automaton::FiniteAutomaton;
using z2kplus::backend::util::automaton::PatternChar;
using z2kplus::backend::test::util::TestUtil;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::test {

namespace {
void checkDFA(const char *pattern, const char **challenges, const bool *expectedResults,
  size_t size) {
  FailRoot fr;
  FiniteAutomaton dfa;
  if (!TestUtil::tryMakeDfa(pattern, &dfa, fr.nest(HERE))) {
    FAIL(fr);
  }

  INFO("The DFA looks like this: " << dfa);
  ReusableString32 rs32;
  for (size_t i = 0; i < size; ++i) {
    auto sv32 = TestUtil::friendlyReset(&rs32, challenges[i]);
    auto finalNode = dfa.start()->tryAdvance(sv32);
    auto actual = finalNode != nullptr && finalNode->accepting();
    auto expected = expectedResults[i];
    INFO("Failed check on " << challenges[i]);
    CHECK(expected == actual);
  }
}
}  // namespace

// Not really sure what file to put this in
TEST_CASE("dfa: Word partitioning","[dfa]") {
  std::vector<std::string_view> challenges = {
    "kosak++",
    "I don't like pie",
    "This \"pain\", no name",
    "I am 🙀Cιηη🔥вση🙀!",
  };

  std::vector<std::vector<std::string_view>> expectedResults = {
    {"kosak", "+", "+"},
    {"I", "don't", "like", "pie"},
    {"This", "\"", "pain", "\"", ",", "no", "name"},
    {"I", "am", "🙀Cιηη🔥вση🙀", "!"}
  };

  REQUIRE(challenges.size() == expectedResults.size());

  for (size_t i = 0; i < challenges.size(); ++i) {
    auto challenge = challenges[i];
    INFO("Challenge is: " << challenge);
    std::vector<std::string_view> tokens;
    WordSplitter::split(challenge, &tokens);
    CHECK(tokens == expectedResults[i]);
  }
}

TEST_CASE("dfa: curious","[dfa]") {
  static std::array<const char *, 4> challenges = {"ABCD", "xABxCDx", "ABABxxCDCD", "zamboni"};
  static std::array<bool, 4> expectedResults = {true, true, true, false};
  static_assert(challenges.size() == expectedResults.size());
  checkDFA("*AB*CD*", challenges.data(), expectedResults.data(), challenges.size());
}


TEST_CASE("dfa: XYZ","[dfa]") {
  static std::array<const char *, 3> challenges = {"xyz", "XYZ", "XYZW"};
  static std::array<bool, 3> expectedResults = {false, true, false};
  static_assert(challenges.size() == expectedResults.size());
  checkDFA("XYZ", challenges.data(), expectedResults.data(), challenges.size());
}

TEST_CASE("dfa: escaped xyz","[dfa]") {
  static std::array<const char *, 2> challenges = {"xyz", "XYZ"};
  static std::array<bool, 2> expectedResults = {true, false};
  static_assert(challenges.size() == expectedResults.size());
  checkDFA(R"(\x\y\z)", challenges.data(), expectedResults.data(), challenges.size());
}

TEST_CASE("dfa: c","[dfa]") {
  static std::array<const char *, 5> challenges = {"c", "C", "ⓒ", "⒞", "x"};
  static std::array<bool, 5> expectedResults = {true, true, true, true, false};
  static_assert(challenges.size() == expectedResults.size());
  checkDFA("c", challenges.data(), expectedResults.data(), challenges.size());
}

TEST_CASE("dfa: xyz","[dfa]") {
  static std::array<const char *, 2> challenges = {"xyz", "XYZ"};
  static std::array<bool, 2> expectedResults = {true, true};
  static_assert(challenges.size() == expectedResults.size());

  checkDFA("xyz", challenges.data(), expectedResults.data(), challenges.size());
}

TEST_CASE("dfa: ?","[dfa]") {
  static std::array<const char *, 7> challenges = {"", "x", "X", "ⓒ", "ⓒ⒞", "🔥", "cinnabon"};
  static std::array<bool, 7> expectedResults = {false, true, true, true, false, true, false};
  static_assert(challenges.size() == expectedResults.size());
  checkDFA("?", challenges.data(), expectedResults.data(), challenges.size());
}

TEST_CASE("dfa: ??","[dfa]") {
  static std::array<const char *, 6> challenges = {"", "x", "ab", "ⓒ⒞", "🔥🔥", "cinnabon"};
  static std::array<bool, 6> expectedResults = {false, false, true, true, true, false};
  static_assert(challenges.size() == expectedResults.size());
  checkDFA("??", challenges.data(), expectedResults.data(), challenges.size());
}

TEST_CASE("dfa: *","[dfa]") {
  static std::array<const char *, 6> challenges = {"", "x", "X", "ⓒ", "ⓒ⒞", "cinnabon"};
  static std::array<bool, 6> expectedResults = {true, true, true, true, true, true};
  static_assert(challenges.size() == expectedResults.size());
  checkDFA("*", challenges.data(), expectedResults.data(), challenges.size());
}

TEST_CASE("dfa: ***","[dfa]") {
  static std::array<const char *, 6> challenges = {"", "x", "X", "ⓒ", "ⓒ⒞", "cinnabon"};
  static std::array<bool, 6> expectedResults = {true, true, true, true, true, true};
  static_assert(challenges.size() == expectedResults.size());
  checkDFA("***", challenges.data(), expectedResults.data(), challenges.size());
}

namespace {
std::array<const char *, 9> cinnabonChallenges = {"cinnabon", "Cinnabon", "cinnbon", "cinn-bon",
    "Cιηηαвση", "Cιηη🔥вση", "🙀Cιηη🔥вση🙀", "🙀xyzCιηη🔥вσηxyz🙀", "cinnamaxibonbon"};
}  // namespace

TEST_CASE("dfa: cinnabon","[dfa]") {
  static std::array<bool, 9> expectedResults = {true, true, false, false, true, false, false, false, false};
  static_assert(cinnabonChallenges.size() == expectedResults.size());
  checkDFA("cinnabon", cinnabonChallenges.data(), expectedResults.data(), cinnabonChallenges.size());
}

TEST_CASE("dfa: cinn?bon","[dfa]") {
  static std::array<bool, 9> expectedResults = {true, true, false, true, true, true, false, false, false};
  static_assert(cinnabonChallenges.size() == expectedResults.size());
  checkDFA("cinn?bon", cinnabonChallenges.data(), expectedResults.data(), cinnabonChallenges.size());
}

TEST_CASE("dfa: cinn*bon","[dfa]") {
  static std::array<bool, 9> expectedResults = {true, true, true, true, true, true, false, false, true};
  static_assert(cinnabonChallenges.size() == expectedResults.size());
  checkDFA("cinn*bon", cinnabonChallenges.data(), expectedResults.data(), cinnabonChallenges.size());
}

TEST_CASE("dfa: cinnabon*","[dfa]") {
  static std::array<bool, 9> expectedResults = {true, true, false, false, true, false, false, false, false};
  static_assert(cinnabonChallenges.size() == expectedResults.size());
  checkDFA("cinnabon*", cinnabonChallenges.data(), expectedResults.data(), cinnabonChallenges.size());
}

TEST_CASE("dfa: *cinnabon","[dfa]") {
  static std::array<bool, 9> expectedResults = {true, true, false, false, true, false, false, false, false};
  static_assert(cinnabonChallenges.size() == expectedResults.size());
  checkDFA("*cinnabon", cinnabonChallenges.data(), expectedResults.data(), cinnabonChallenges.size());
}

TEST_CASE("dfa: *cinnabon*","[dfa]") {
  static std::array<bool, 9> expectedResults = {true, true, false, false, true, false, false, false, false};
  static_assert(cinnabonChallenges.size() == expectedResults.size());
  checkDFA("*cinnabon*", cinnabonChallenges.data(), expectedResults.data(), cinnabonChallenges.size());
}

TEST_CASE("dfa: *cinn?bon*","[dfa]") {
  static std::array<bool, 9> expectedResults = {true, true, false, true, true, true, true, true, false};
  static_assert(cinnabonChallenges.size() == expectedResults.size());
  checkDFA("*cinn?bon*", cinnabonChallenges.data(), expectedResults.data(), cinnabonChallenges.size());
}

TEST_CASE("dfa: *cinn*bon*","[dfa]") {
  static std::array<bool, 9> expectedResults = {true, true, true, true, true, true, true, true, true};
  static_assert(cinnabonChallenges.size() == expectedResults.size());
  checkDFA("*cinn*bon*", cinnabonChallenges.data(), expectedResults.data(), cinnabonChallenges.size());
}

static void testAcceptEverything(std::string_view pattern, bool expectedResult) {
  FailRoot fr;
  FiniteAutomaton dfa;
  if (!TestUtil::tryMakeDfa(pattern, &dfa, fr.nest(HERE))) {
    FAIL(fr);
  }

  CHECK(dfa.start()->acceptsEverything() == expectedResult);
}

TEST_CASE("dfa: ? does not accept everything","[dfa]") {
  testAcceptEverything("?", false);
}

TEST_CASE("dfa: ?? does not accept everything","[dfa]") {
  testAcceptEverything("??", false);
}

TEST_CASE("dfa: ?* does not accept everything","[dfa]") {
  testAcceptEverything("?*", false);
}

TEST_CASE("dfa: * accepts everything","[dfa]") {
  testAcceptEverything("*", true);
}

TEST_CASE("dfa: ** accepts everything","[dfa]") {
  testAcceptEverything("**", true);
}

TEST_CASE("dfa: ***** accepts everything","[dfa]") {
  testAcceptEverything("******", true);
}

}  // namespace z2kplus::backend::test
