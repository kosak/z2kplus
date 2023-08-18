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

#include "z2kplus/backend/queryparsing/util.h"

#include "kosak/coding/dumping.h"
#include "kosak/coding/text/conversions.h"
#include "z2kplus/backend/util/automaton/automaton.h"

namespace z2kplus::backend::queryparsing {

using kosak::coding::FailFrame;
using kosak::coding::toString;
using kosak::coding::text::tryConvertUtf8ToUtf32;
using z2kplus::backend::util::automaton::FiniteAutomaton;
using z2kplus::backend::util::automaton::PatternChar;

#define HERE KOSAK_CODING_HERE

void WordSplitter::split(std::string_view text, std::vector<std::string_view> *tokens) {
  auto isAlphabet = [](unsigned char uch) {
    return (uch >= 'A' && uch <= 'Z') ||
        (uch >= 'a' && uch <= 'z') ||
        (uch >= '0' && uch <= '9') ||
        (uch >= 0x80 && uch <= 0xff);
  };
  const char *current = text.begin();
  const char *end = text.end();
  while (current != end) {
    auto uch = (unsigned char)*current;
    // skip "whitespace": control characters, space, and DEL
    if (uch < 0x20 || uch == ' ' || uch == 0x7f) {
      ++current;
      continue;
    }

    // "words" start with an "alphabet" character and continue with an "alphabet" character or
    // apostrophe.
    if (isAlphabet(uch)) {
      const char *wordStart = current++;
      while (current != end) {
        uch = (unsigned char) *current;
        if (!isAlphabet(uch) && uch != '\'') {
          break;
        }
        ++current;
      }

      // But we may have sucked in a word with trailing apostrophes, and we only allow
      // inner apostrophes. So back them out.
      while (current[-1] == '\'') {
        --current;
      }
      tokens->emplace_back(wordStart, current - wordStart);
      continue;
    }

    // anything else is just a single character
    tokens->emplace_back(current, 1);
    ++current;
  }
}

// Here's what we're going to go with. Backslash has two roles:
// 1. Escape the metacharacters ? and * and
// 2. Control the "forceMatch" setting to set the exactness of unescaped lowercase characters.
//
// 1. Backslash allows you to escape metacharacters. Also
// cinnabon - loose match for all characters
// cinna\bon - turns on forceExact: so all characters must exactly match.
// cinnaBon - turns on forceExact: so all characters must exactly match.
// cinna\Bon - does not turn on forceExact.
//
// Put another way:
// An unescaped uppercase character or an escaped lowercase character forces exact match for the
// whole string.
void
WordSplitter::translateToPatternChar(std::u32string_view sr, std::vector<PatternChar> *result) {
  // Scan escapes
  bool forceExact = false;
  for (size_t i = 0; i < sr.size(); ++i) {
    auto ch = sr[i];
    if (ch != '\\') {
      // No backslash. The simpler case.
      if (isupper(ch)) {
        forceExact = true;
      }
      continue;
    }

    // Skip over the backslash.
    if (++i == sr.size()) {
      break;
    }
    // If the next character is lowercase, force exact
    if (islower(sr[i])) {
      forceExact = true;
    }
  }

  for (size_t i = 0; i < sr.size(); ++i) {
    auto ch = sr[i];
    if (ch == '\\') {
      // If there is an unmatched backslash at the end, we'll just keep it.
      if (i != sr.size() - 1) {
        ++i;
      }
      result->push_back(PatternChar::create(sr[i], !forceExact));
      continue;
    }

    if (ch == '?') {
      result->push_back(PatternChar::createMatchOne());
      continue;
    }

    if (ch == '*') {
      result->push_back(PatternChar::createMatchN());
      continue;
    }

    result->push_back(PatternChar::create(ch, !forceExact));
  }
}

//std::string toHumanReadable(const std::vector<util::automaton::PatternChar> &pattern) {
//  vector<pair<CharType, string>> elements;
//  for (const auto &pc : pattern) {
//    auto srToUse = pc.asStringView();
//    switch (pc.type()) {
//      case CharType::MatchOne: srToUse = "?"; break;
//      case CharType::MatchMany: srToUse = "*"; break;
//      default: break; // nothing
//    }
//    if (elements.empty() || elements.back().first != pc.type()) {
//      elements.emplace_back(pc.type(), srToUse);
//      continue;
//    }
//    elements.back().second.append(srToUse.data(), srToUse.size());
//  }
//  return toString(elements);
//}

}  //  namespace z2kplus::backend::queryparsing
