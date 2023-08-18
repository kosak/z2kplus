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

#include <ostream>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/reverse_index/trie/frozen_node.h"
#include "z2kplus/backend/reverse_index/builder/common.h"

namespace z2kplus::backend::reverse_index::builder {
class TrieBuilderNode {
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::reverse_index::trie::FrozenNode FrozenNode;
  typedef z2kplus::backend::reverse_index::builder::SimpleAllocator SimpleAllocator;
public:
  TrieBuilderNode();
  TrieBuilderNode(std::u32string prefix, std::vector<wordOff_t> wordsHere,
      char32_t dynamicTransition, std::unique_ptr<TrieBuilderNode> dynamicChild,
      std::vector<std::pair<char32_t, FrozenNode *>> frozenTransitions);
  DISALLOW_COPY_AND_ASSIGN(TrieBuilderNode);
  DISALLOW_MOVE_COPY_AND_ASSIGN(TrieBuilderNode);
  ~TrieBuilderNode();

  bool tryInsert(const std::u32string_view &probe, const wordOff_t *begin, size_t size,
      SimpleAllocator *alloc, const FailFrame &ff);

  bool tryFreeze(SimpleAllocator *alloc, FrozenNode **result, const FailFrame &ff);

private:
  bool tryInsertHelper(std::u32string_view probe, const wordOff_t *begin, size_t size,
      SimpleAllocator *alloc, const FailFrame &ff);

  std::u32string prefix_;
  std::vector<wordOff_t> wordsHere_;

  // valid if dynamicChild_ is not nullptr
  char32_t dynamicTransition_ = 0;
  std::unique_ptr<TrieBuilderNode> dynamicChild_;

  std::vector<std::pair<char32_t, FrozenNode*>> frozenTransitions_;
};
}   // namespace z2kplus::backend::reverse_index::builder
