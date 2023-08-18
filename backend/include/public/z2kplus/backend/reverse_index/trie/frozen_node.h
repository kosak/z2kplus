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
#include <string_view>
#include "z2kplus/backend/reverse_index/types.h"
#include "z2kplus/backend/util/misc.h"
#include "z2kplus/backend/util/myallocator.h"
#include "z2kplus/backend/util/relative.h"
#include "z2kplus/backend/util/automaton/automaton.h"

namespace z2kplus::backend::reverse_index::trie {
// This class points to a variable-sized object, described below in
// internal::FrozenNodeDocumentation. You interpret this class with a FrozenNodeView
// (defined anonymously in frozen_node.cc)
class FrozenNode {
  typedef kosak::coding::FailFrame FailFrame;
  template<typename T>
  using RelativePtr = z2kplus::backend::util::RelativePtr<T>;
  typedef z2kplus::backend::util::automaton::FiniteAutomaton FiniteAutomaton;

public:
  bool tryFind(std::u32string_view probe,
      std::pair<const wordOff_t *, const wordOff_t *> *result) const;

  void findMatching(const FiniteAutomaton &dfa,
      const kosak::coding::Delegate<void, const wordOff_t *, const wordOff_t *> &callback) const;

  bool tryDump(std::ostream &s, std::string *debugReadable, const FailFrame &ff) const;

  // The fixed part of the data structure
  uint32_t prefixSize_;
  uint32_t numWordsHere_;
  uint32_t numTransitions_;
  // These lengths are all wrong (they are written as length 0), so you actually have to
  // dynamically step through the rest of this type.
  // Incoming prefix to this node.
  char32_t prefix_[0];
  // // The words at this node. Has size numWordsHere_.
  // wordOff_t wordsHere[numWordsHere_];
  // // The keys of the outgoing transitions. Has size numTransitions_.
  // char32_t transitionKeys_[numTranstitions_];
  // // The transitions themselves. Has size numTransitions_. Since they are aligned to 64 bits,
  // // We might need a 32 bit word of padding here.
  // uint32_t padding[0 or 1];
  // RelativePtr<FrozenNode> transitions[numTransitions_]
};
}  // namespace z2kplus::backend::reverse_index::trie
