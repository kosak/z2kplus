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

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/delegate.h"
#include "z2kplus/backend/reverse_index/types.h"
#include "z2kplus/backend/util/automaton/automaton.h"

namespace z2kplus::backend::reverse_index::trie {

class DynamicNode {
public:
  typedef std::map<char32_t, DynamicNode> transitions_t;
  typedef z2kplus::backend::util::automaton::FiniteAutomaton FiniteAutomaton;
  typedef z2kplus::backend::util::automaton::DFANode DFANode;

  template<typename R, typename ...ARGS>
  using Delegate = kosak::coding::Delegate<R, ARGS...>;

  DynamicNode();
  DISALLOW_COPY_AND_ASSIGN(DynamicNode);
  DECLARE_MOVE_COPY_AND_ASSIGN(DynamicNode);
  ~DynamicNode();

  bool tryFind(std::u32string_view probe,
      std::pair<const wordOff_t *, const wordOff_t *> *result) const;
  void insert(std::u32string_view probe, const wordOff_t *begin, size_t size);

  void findMatching(const FiniteAutomaton &dfa,
      const kosak::coding::Delegate<void, const wordOff_t *, const wordOff_t *> &callback) const {
    findMatchingHelper(dfa.start(), callback);
  }

  void dump(std::ostream &s, std::u32string *prefix) const;

private:
  DynamicNode(std::u32string &&prefix, std::vector<wordOff_t> &&wordsHere,
      transitions_t &&transitions);

  void findMatchingHelper(const DFANode *dfaNode,
      const kosak::coding::Delegate<void, const wordOff_t *, const wordOff_t *> &callback) const;

  void insertHelper(std::u32string_view probe, const wordOff_t *begin, size_t size);

  bool isPlaceholder() const;

  std::u32string prefix_;
  std::vector<wordOff_t> wordsHere_;
  transitions_t transitions_;
};

}  // namespace z2kplus::backend::reverse_index::trie
