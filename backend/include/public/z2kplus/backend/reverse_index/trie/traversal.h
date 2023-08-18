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

#include <utility>
#include <vector>
#include "kosak/coding/coding.h"
#include "z2kplus/backend/reverse_index/trie/frozen_trie.h"
#include "z2kplus/backend/util/automaton/automaton.h"

namespace z2kplus::backend::reverse_index::trie {

void findMatchingWords(const FreezableTrie &trie,
  const z2kplus::backend::util::automaton::FiniteAutomaton &dfa,
  std::vector<std::pair<const uint32*, const uint32*>> *ranges);

}  // namespace z2kplus::backend::reverse_index::trie
