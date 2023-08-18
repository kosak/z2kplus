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

#pragma once
#include <regex>
#include <string_view>
#include <vector>
#include "kosak/coding/coding.h"
#include "z2kplus/backend/util/automaton/automaton.h"

namespace z2kplus::backend::queryparsing {

class WordSplitter {
public:
  using PatternChar = util::automaton::PatternChar;
  WordSplitter() = delete;

  static void split(std::string_view text, std::vector<std::string_view> *tokens);

  // Translate query utterances into a vector of PatternChars according to our convention:
  // Loose vs strict:
  // - if the string has any upper case characters, or any backslashed lowercase letters, we make
  //   the whole string a strict match. Otherwise, the whole string is a loose match.
  // Unescaped ? and * characters become MatchOne and MatchMany
  static void
  translateToPatternChar(std::u32string_view sr, std::vector<PatternChar> *result);
};
// std::string toHumanReadable(const std::vector<util::automaton::PatternChar> &pattern);
}  // namespace z2kplus::backend::queryparsing
