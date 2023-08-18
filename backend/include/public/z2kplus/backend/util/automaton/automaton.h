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

#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "z2kplus/backend/util/misc.h"

namespace z2kplus::backend::util::automaton {
// We implement an NDFA, as well as NDFA-to-DFA conversion. This is for the purpose of efficiently
// implementing our little globbing language. This approach might be overkill, but the advantage is
// that the language can be made more powerful later, if desired. Language elements are PatternChar
// with the rules documented at PatternChar.
class DFANode;
class PatternChar;
class FiniteAutomaton {
public:
  FiniteAutomaton();
  FiniteAutomaton(const PatternChar *begin, size_t patternSize, std::string description);
  DECLARE_MOVE_COPY_AND_ASSIGN(FiniteAutomaton);
  ~FiniteAutomaton();

  const DFANode *start() const { return start_; }
  const std::string &description() const { return description_; }

private:
  // All the nodes.
  std::vector<DFANode> nodes_;
  // The start node.
  const DFANode *start_ = nullptr;
  // Human-readable description.
  std::string description_;

  friend std::ostream &operator<<(std::ostream &s, const FiniteAutomaton &o);
};

namespace internal {
// Exact means the character matches only itself.
// Loose means that the character matches its upper case sibling, or Unicode characters that sort
// of resemble it (e.g. c could match C or 𝒄,𝓬,𝕔,𝚌,𝖼,𝗰,𝙘...)
// MatchOne matches exactly one character ('?' in glob syntax)
// MatchN matches arbitrarily many characters including a zero-length sequence ('*' in glob syntax)
enum class CharType {
  Exact, Loose, MatchOne, MatchN
};
std::ostream &operator<<(std::ostream &s, CharType ct);
}  // namespace internal

class PatternChar {
public:
  static PatternChar create(char32_t ch, bool loose) {
    auto type = loose && ch >= 'a' && ch <= 'z' ? internal::CharType::Loose : internal::CharType::Exact;
    return PatternChar(type, ch);
  }

  static PatternChar createMatchOne() { return PatternChar(internal::CharType::MatchOne, 0); }
  static PatternChar createMatchN() { return PatternChar(internal::CharType::MatchN, 0); }

  // std::string_view asStringView() const { return {chars_, size_}; }

  internal::CharType type() const { return type_; }
  char32_t ch() const { return ch_; }

private:
  PatternChar(internal::CharType type, char32_t ch) : type_(type), ch_(ch) {}

  internal::CharType type_ = internal::CharType::Exact;
  char32_t ch_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const PatternChar &o);
};

class DFANode {
public:
  typedef std::pair<char32_t, const DFANode*> transition_t;

  DFANode();
  DFANode(bool accepting, std::vector<transition_t> transitions, const DFANode *otherwise);
  ~DFANode();

  const DFANode *tryAdvance(char32_t key) const;
  const DFANode *tryAdvance(std::u32string_view key) const;
  void tryAdvanceMulti(std::u32string_view keys, const DFANode **result) const;

  bool acceptsEverything() const;

  bool accepting() const { return accepting_; }
  const std::vector<transition_t> &transitions() const { return transitions_; }
  const DFANode *otherwise() const { return otherwise_; }

private:
  // If this node is an accepting state.
  bool accepting_ = false;
  std::vector<transition_t> transitions_;
  const DFANode *otherwise_ = nullptr;

  friend class FiniteAutomaton;
};
}  // namespace z2kplus::backend::util::automaton
