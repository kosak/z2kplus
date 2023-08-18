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

#include "z2kplus/backend/reverse_index/builder/trie_builder.h"

#include "kosak/coding/failures.h"

#define HERE KOSAK_CODING_HERE

using kosak::coding::FailFrame;
using z2kplus::backend::util::RelativePtr;

namespace z2kplus::backend::reverse_index::builder {
TrieBuilderNode::TrieBuilderNode() = default;
TrieBuilderNode::TrieBuilderNode(std::u32string prefix, std::vector<wordOff_t> wordsHere,
    char32_t dynamicTransition, std::unique_ptr<TrieBuilderNode> dynamicChild,
    std::vector<std::pair<char32_t, FrozenNode *>> frozenTransitions) : prefix_(std::move(prefix)),
    wordsHere_(std::move(wordsHere)), dynamicTransition_(dynamicTransition),
    dynamicChild_(std::move(dynamicChild)), frozenTransitions_(std::move(frozenTransitions)) {
}
TrieBuilderNode::~TrieBuilderNode() = default;

bool TrieBuilderNode::tryInsert(const std::u32string_view &probe, const wordOff_t *begin, size_t size,
    SimpleAllocator *alloc, const FailFrame &ff) {
  if (size == 0) {
    // Nothing to append.
    return true;
  }
  // Find the point where 'prefix_' differs from 'probe' (could also be at the end of either).
  auto mm = std::mismatch(prefix_.begin(), prefix_.end(), probe.begin(), probe.end());
  size_t diffIndex = mm.first - prefix_.begin();

  if (diffIndex == prefix_.size()) {
    // Prefix satisfied, so do remainder of work starting from this node.
    return tryInsertHelper(probe.substr(diffIndex), begin, size, alloc, ff.nest(HERE));
  }

  // Need to split this node at 'diffIndex'.
  std::u32string_view pfxView(prefix_);
  auto cloneTransition = pfxView[diffIndex];
  auto cloneRemainder = pfxView.substr(diffIndex + 1);

  // We will make a near-clone of ourselves with the same outgoing transitions.
  // It will have a prefix of "cloneRemainder". Then we will hollow out our own state, set our
  // prefix to "commonPrefix" and with a transition on "cloneTransition" to our clone.
  // Then we will basically run the above logic again.
  auto clone = std::make_unique<TrieBuilderNode>(std::u32string(cloneRemainder), std::move(wordsHere_),
      dynamicTransition_, std::move(dynamicChild_), std::move(frozenTransitions_));
  // Now fix up myself, e.g. sanitizing after the moves, and point to the child.
  prefix_.erase(diffIndex, std::u32string::npos);  // The common prefix
  dynamicTransition_ = cloneTransition;
  dynamicChild_ = std::move(clone);
  wordsHere_.clear();
  frozenTransitions_.clear();

  // Now I can just hand off to the insertHelper logic
  return tryInsertHelper(probe.substr(diffIndex), begin, size, alloc, ff.nest(HERE));
}

bool TrieBuilderNode::tryInsertHelper(std::u32string_view probe, const wordOff_t *begin, size_t size,
    SimpleAllocator *alloc, const FailFrame &ff) {
  // Three cases:
  // 1. If probe is empty then we're appending right here.
  // 2. Otherwise, if there is an existing transition on the first character of probe, then recurse
  //    on that transition
  // 3. Otherwise, create that transition.

  if (probe.empty()) {
    wordsHere_.insert(wordsHere_.end(), begin, begin + size);
    return true;
  }
  auto transition = probe[0];
  auto remainder = probe.substr(1);
  if (dynamicChild_ != nullptr && transition == dynamicTransition_) {
    // Recurse.
    return dynamicChild_->tryInsert(remainder, begin, size, alloc, ff.nest(HERE));
  }

  // New transition is here. First freeze our dynamic child, if we have one.
  if (dynamicChild_ != nullptr) {
    // we could assert that transition > dynamicTransition_
    FrozenNode *frozenNode;
    if (!dynamicChild_->tryFreeze(alloc, &frozenNode, ff.nest(HERE))) {
      return false;
    }
    frozenTransitions_.emplace_back(dynamicTransition_, frozenNode);
    dynamicTransition_ = 0;  // hygeine
    dynamicChild_.reset();
  }

  // Now make a new dynamic child.
  std::vector<wordOff_t> nextWords(begin, begin + size);
  dynamicTransition_ = transition;
  dynamicChild_ = std::make_unique<TrieBuilderNode>(std::u32string(remainder), std::move(nextWords),
      0, nullptr, std::vector<std::pair<char32_t, FrozenNode*>>());
  return true;
}

bool TrieBuilderNode::tryFreeze(SimpleAllocator *alloc, FrozenNode **result, const FailFrame &ff) {
  if (dynamicChild_ != nullptr) {
    // we could assert that transition > dynamicTransition_
    FrozenNode *frozenChild;
    if (!dynamicChild_->tryFreeze(alloc, &frozenChild, ff.nest(HERE))) {
      return false;
    }
    frozenTransitions_.emplace_back(dynamicTransition_, frozenChild);
    dynamicTransition_ = 0;  // hygeine
    dynamicChild_.reset();
  }
  FrozenNode *newNode;
  char32_t *prefix;
  wordOff_t *wordsHere;
  char32_t *transitionKeys;
  RelativePtr<FrozenNode> *transitions;
  if (!alloc->tryAllocate(1, &newNode, ff.nest(HERE)) ||
      !alloc->tryAllocate(prefix_.size(), &prefix, ff.nest(HERE)) ||
      !alloc->tryAllocate(wordsHere_.size(), &wordsHere, ff.nest(HERE)) ||
      !alloc->tryAllocate(frozenTransitions_.size(), &transitionKeys, ff.nest(HERE)) ||
      !alloc->tryAllocate(frozenTransitions_.size(), &transitions, ff.nest(HERE))) {
    return false;
  }
  newNode->prefixSize_ = prefix_.size();
  newNode->numWordsHere_ = wordsHere_.size();
  newNode->numTransitions_ = frozenTransitions_.size();
  std::copy(prefix_.begin(), prefix_.end(), prefix);
  std::copy(wordsHere_.begin(), wordsHere_.end(), wordsHere);
  for (size_t i = 0; i != frozenTransitions_.size(); ++i) {
    const auto &ft = frozenTransitions_[i];
    transitionKeys[i] = ft.first;
    transitions[i].set(ft.second);
  }
  *result = newNode;
  return true;
}
}   // namespace z2kplus::backend::reverse_index::builder
