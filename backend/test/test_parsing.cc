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
#include "catch/catch.hpp"
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "z2kplus/backend/queryparsing/parser.h"
#include "z2kplus/backend/reverse_index/iterators/iterator_common.h"
#include "z2kplus/backend/test/util/test_util.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::toString;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::queryparsing::parse;
using z2kplus::backend::shared::Zephyrgram;
using z2kplus::backend::reverse_index::FieldMask;
using z2kplus::backend::reverse_index::WordInfo;
using z2kplus::backend::reverse_index::iterators::WordIterator;
using z2kplus::backend::reverse_index::iterators::ZgramIterator;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::test {

namespace {
bool testParse(std::string_view raw, bool emptyMeansEverything, std::string_view expectedCooked,
  const FailFrame &ff) {
  std::unique_ptr<ZgramIterator> iterator;
  if (!parse(raw, emptyMeansEverything, &iterator, ff.nest(HERE))) {
    return false;
  }
  auto actualCooked = toString(*iterator);
  if (actualCooked != expectedCooked) {
    return ff.failf(HERE, "raw %o\n"
                          "expected %o\n",
                          "actual %o", raw, expectedCooked, actualCooked);
  }
  return true;
}

bool testFail(std::string_view raw, bool emptyMeansEverything, const FailFrame &ff) {
  std::unique_ptr<ZgramIterator> iterator;
  FailRoot fakeRoot;
  if (parse(raw, emptyMeansEverything, &iterator, fakeRoot.nest(HERE))) {
    return ff.fail(HERE, "Should have failed but didn't");
  }
  debug("BTW, the (expected) parse failures are %o", fakeRoot);
  return true;
}

}  // namespace

// Words next to each other
TEST_CASE("parsing: implicit and","[parsing]") {
  FailRoot fr;
  // two words
  testParse("corey kosak", true,
    R"(And([Adapt(Pattern(instance|body, corey)), Adapt(Pattern(instance|body, kosak))]))",
    fr.nest(HERE));
  // three words
  testParse("corey louis kosak", true,
    R"(And([Adapt(Pattern(instance|body, corey)), Adapt(Pattern(instance|body, louis)), Adapt(Pattern(instance|body, kosak))]))",
      fr.nest(HERE));
  // three words with field specifiers
  testParse("sender:corey instance:louis signature:kosak", true,
    R"(And([Adapt(Pattern(sender, corey)), Adapt(Pattern(instance, louis)), Adapt(Pattern(signature, kosak))]))",
      fr.nest(HERE));
  if (!fr.ok()) {
    FAIL(fr);
  }
}

// let's test something like instance, body: kosak

// Apostrophes
TEST_CASE("parsing: apostrophe","[parsing]") {
  FailRoot fr;
  // single word
  testParse("kosak", true,
    R"(Adapt(Pattern(instance|body, kosak)))",
    fr.nest(HERE));
  // contraction: should be a single word
  testParse("k'osak", true,
    R"(Adapt(Pattern(instance|body, k'osak)))",
      fr.nest(HERE));
  // contraction: should be a single word
  testParse("k'osa'k", true,
    R"(Adapt(Pattern(instance|body, k'osa'k)))",
      fr.nest(HERE));
  // apostrophe at front: two words
  testParse("'kosak", true,
    R"(Near(1, [Pattern(instance|body, '), Pattern(instance|body, kosak)]))",
      fr.nest(HERE));
  // apostrophe at end: two words
  testParse("kosak'", true,
    R"(Near(1, [Pattern(instance|body, kosak), Pattern(instance|body, ')]))",
      fr.nest(HERE));
  // apostrophe on both sides: three words
  testParse("'kosak'", true,
    R"(Near(1, [Pattern(instance|body, '), Pattern(instance|body, kosak), Pattern(instance|body, ')]))",
      fr.nest(HERE));
  if (!fr.ok()) {
    FAIL(fr);
  }
}

// Quotation Marks
TEST_CASE("parsing: quotation mark","[parsing]") {
  FailRoot fr;
  // quote at end: fail
  testFail(R"(kosak")", true, fr.nest(HERE));
  // quote at front: fail
  testFail(R"("kosak)", true, fr.nest(HERE));
  // empty string
  testParse(R"("")", true,
    "PopOrNot(pop=(none), unpop=instance|body)",
      fr.nest(HERE));
  // enclosed in quotes: one word
  testParse(R"("kosak")", true,
    R"(Adapt(Pattern(instance|body, kosak)))",
      fr.nest(HERE));
  // two words enclosed in quotes: Near 1
  testParse(R"("corey kosak")", true,
    R"(Near(1, [Pattern(instance|body, corey), Pattern(instance|body, kosak)]))",
      fr.nest(HERE));
  // field specifiers apply to the whole quoted string
  testParse(R"(signature:"corey kosak")", true,
    R"(Near(1, [Pattern(signature, corey), Pattern(signature, kosak)]))",
      fr.nest(HERE));

  // field specifiers lose their meaning inside quotes
  testParse(R"("sender:kosak")", true,
    R"(Near(1, [Pattern(instance|body, sender), Pattern(instance|body, :), Pattern(instance|body, kosak)]))",
      fr.nest(HERE));

  // paren in quotes
  testParse(R"xxx("kosak)")xxx", true,
    R"(Near(1, [Pattern(instance|body, kosak), Pattern(instance|body, ))]))",
      fr.nest(HERE));

  // anchor and paren in quotes
  testParse(R"xxx("kosak)$")xxx", true,
    R"(Near(1, [Pattern(instance|body, kosak), Pattern(instance|body, )), Pattern(instance|body, $)]))",
      fr.nest(HERE));

  if (!fr.ok()) {
    FAIL(fr);
  }
}

// Tilde is a kind of quote that means "near 3"
TEST_CASE("parsing: tilde","[parsing]") {
  FailRoot fr;
  // tilde at end: fail
  testFail("kosak~", true, fr.nest(HERE));
  // tilde at front: fail
  testFail("~kosak", true, fr.nest(HERE));
  // empty string
  testParse("~~", true,
    "PopOrNot(pop=(none), unpop=instance|body)",
      fr.nest(HERE));
  // enclosed in tildes: one word
  testParse("~kosak~", true,
    R"(Adapt(Pattern(instance|body, kosak)))",
      fr.nest(HERE));
  // two words enclosed in quotes: near
  testParse("~corey kosak~", true,
    R"(Near(3, [Pattern(instance|body, corey), Pattern(instance|body, kosak)]))",
      fr.nest(HERE));
  // field specifiers apply to the whole quoted string
  testParse("signature:~corey kosak~", true,
    R"(Near(3, [Pattern(signature, corey), Pattern(signature, kosak)]))",
      fr.nest(HERE));
  // field specifiers lose their meaning inside tildes
  testParse("~sender:kosak~", true,
    R"(Near(3, [Pattern(instance|body, sender), Pattern(instance|body, :), Pattern(instance|body, kosak)]))",
      fr.nest(HERE));
  if (!fr.ok()) {
    FAIL(fr);
  }
}

TEST_CASE("parsing: edge cases","[parsing]") {
  FailRoot fr;
  // Should find only empty strings on the signature field
  testParse(R"(signature:"")", true,
    "PopOrNot(pop=(none), unpop=signature)",
    fr.nest(HERE));
  if (!fr.ok()) {
    FAIL(fr);
  }
}

//
//
//
//  // quote at front: two words
//  testParse("\"kosak", "kosak");
//  // quote at end: two words
//  testParse("kosak\"", "kosak");
//  // quoted at both ends: one word
//  testParse("\"kosak\"", "kosak");
//
//  testParse("Corey Kosak", "corey kosak");
//  testParse("Corey Kosak can't contract", "corey kosak");
//  testParse("not kosak", "corey kosak");
//  testParse("not not kosak", "corey kosak");
//  testParse("kosak and not signature:kosak",
//    "^And([^Adapt(Pattern(BODY:\"kosak\")), ^Not(Adapt(Pattern(SIGNATURE:\"kosak\")))])");

}  // namespace z2kplus::backend::test
