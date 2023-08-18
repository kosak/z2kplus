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
#include "z2kplus/backend/reverse_index/trie/dynamic_node.h"
#include "z2kplus/backend/reverse_index/trie/frozen_node.h"
#include "z2kplus/backend/reverse_index/types.h"
#include "z2kplus/backend/util/automaton/automaton.h"
#include "z2kplus/backend/util/misc.h"
#include "z2kplus/backend/util/relative.h"

namespace z2kplus::backend::reverse_index::trie {

class FrozenTrie {
  typedef z2kplus::backend::util::automaton::FiniteAutomaton FiniteAutomaton;
  template<typename T>
  using RelativePtr = z2kplus::backend::util::RelativePtr<T>;

public:
  FrozenTrie() = default;
  explicit FrozenTrie(const FrozenNode *root) : root_(root) {}
  DISALLOW_COPY_AND_ASSIGN(FrozenTrie);
  DEFINE_MOVE_COPY_AND_ASSIGN(FrozenTrie);
  ~FrozenTrie() = default;

  bool tryFind(std::u32string_view probe,
      std::pair<const wordOff_t *, const wordOff_t *> *result) const {
    return root_.get()->tryFind(probe, result);
  }

  void findMatching(const FiniteAutomaton &dfa,
      const kosak::coding::Delegate<void, const wordOff_t *, const wordOff_t *> &callback) const {
    root_.get()->findMatching(dfa, callback);
  }

private:
  RelativePtr<const FrozenNode> root_;

  friend std::ostream &operator<<(std::ostream &s, const FrozenTrie &o);
};
}  // namespace z2kplus::backend::reverse_index::trie
