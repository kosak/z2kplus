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

#include "z2kplus/backend/reverse_index/trie/dynamic_node.h"

#include <vector>
#include "kosak/coding/dumping.h"
#include "z2kplus/backend/reverse_index/trie/frozen_node.h"
namespace z2kplus::backend::reverse_index::trie {

using kosak::coding::Hexer;
using z2kplus::backend::reverse_index::trie::FrozenNode;

DynamicNode::DynamicNode() = default;
DynamicNode::DynamicNode(std::u32string &&prefix, std::vector<wordOff_t> &&wordsHere,
    transitions_t &&transitions) : prefix_(std::move(prefix)), wordsHere_(std::move(wordsHere)),
    transitions_(std::move(transitions)) {}
DynamicNode::DynamicNode(DynamicNode &&) noexcept = default;
DynamicNode &DynamicNode::operator=(DynamicNode &&other) noexcept = default;
DynamicNode::~DynamicNode() = default;

bool DynamicNode::tryFind(std::u32string_view probe,
    std::pair<const wordOff_t *, const wordOff_t *> *result) const {
  if (probe.substr(0, prefix_.size()) != prefix_) {
    return false;
  }
  auto residual = probe.substr(prefix_.size());
  if (residual.empty()) {
    if (wordsHere_.empty()) {
      return false;
    }
    result->first = wordsHere_.data();
    result->second = wordsHere_.data() + wordsHere_.size();
    return true;
  }
  auto ip = transitions_.find(residual[0]);
  if (ip == transitions_.end()) {
    return false;
  }
  return ip->second.tryFind(residual.substr(1), result);
}

void DynamicNode::insert(std::u32string_view probe, const wordOff_t *begin, size_t size) {
  if (size == 0) {
    // We don't bother inserting empty wordlists.
    return;
  }

  if (isPlaceholder()) {
    // If I am the placeholder node, then we can set these fields directly.
    prefix_ = probe;
    wordsHere_ = std::vector(begin, begin + size);
    return;
  }

  // Find the point where 'prefix_' differs from 'probe' (could also be at the end of either).
  auto mm = std::mismatch(prefix_.begin(), prefix_.end(), probe.begin(), probe.end());
  size_t diffIndex = mm.first - prefix_.begin();

  if (diffIndex == prefix_.size()) {
    // Prefix satisfied, so do remainder of work starting from this node.
    return insertHelper(probe.substr(diffIndex), begin, size);
  }

  // Need to split this node at 'diffIndex'.
  std::u32string_view pfxView(prefix_);
  // auto commonPrefix = pfxView.substr(diffIndex);
  auto cloneTransition = pfxView[diffIndex];
  auto cloneRemainder = pfxView.substr(diffIndex + 1);

  // We will make a near-clone of ourselves with the same outgoing transitions.
  // It will have a prefix of "cloneRemainder". Then we will hollow out our own state, set our
  // prefix to "commonPrefix" and with a transition on "cloneTransition" to our clone.
  // Then we will basically run the above logic again.

  DynamicNode clone(std::u32string(cloneRemainder), std::move(wordsHere_), std::move(transitions_));
  // Now fix up myself, e.g. sanitizing after the moves, and point to the child.
  prefix_.erase(diffIndex, std::u32string::npos);  // commonPrefix
  transitions_.clear();
  wordsHere_.clear();
  transitions_.try_emplace(cloneTransition, std::move(clone));

  // Now I can just hand off to the insertHelper logic
  insertHelper(probe.substr(diffIndex), begin, size);
}

void DynamicNode::insertHelper(std::u32string_view probe, const wordOff_t *begin, size_t size) {
  // Three cases:
  // 1. If probe is empty then we're appending right here.
  // 2. Otherwise, if there is an existing transition on the first character of probe, then recurse
  //    on that transition
  // 3. Otherwise, create that transition.
  if (probe.empty()) {
    wordsHere_.insert(wordsHere_.end(), begin, begin + size);
    return;
  }
  auto transition = probe[0];
  auto remainder = probe.substr(1);

  // Either recurse to an existing node or create a new one
  auto [ip, inserted] = transitions_.try_emplace(transition);
  auto *nextNode = &ip->second;
  if (!inserted) {
    // Case 2
    return nextNode->insert(remainder, begin, size);
  }
  // Fill in the default-constructed DynamicNode just created
  nextNode->prefix_ = remainder;
  nextNode->wordsHere_ = std::vector<wordOff_t>(begin, begin + size);
  // nextNode->transitions_  // remains empty
}

void DynamicNode::dump(std::ostream &s, std::u32string *prefix) const {
  s << "[DynamicNode::dump not implemented]\n";
}

void DynamicNode::findMatchingHelper(const DFANode *dfaNode,
    const Delegate<void, const wordOff_t *, const wordOff_t *> &callback) const {
  const auto *dfaToUse = dfaNode->tryAdvance(prefix_);
  if (dfaToUse == nullptr) {
    return;
  }

  if (!wordsHere_.empty() && dfaToUse->accepting()) {
    callback(&*wordsHere_.begin(), &*wordsHere_.end());
  }

  if (transitions_.empty()) {
    return;
  }

  auto size = transitions_.size();
  char32_t transitionKeys[size];
  const DynamicNode *childps[size];
  const DFANode *childDfas[size];
  size_t destIndex = 0;
  for (const auto &ip : transitions_) {
    transitionKeys[destIndex] = ip.first;
    childps[destIndex] = &ip.second;
    ++destIndex;
  }
  std::u32string_view sv(transitionKeys, size);
  dfaToUse->tryAdvanceMulti(sv, childDfas);
  for (size_t i = 0; i < size; ++i) {
    const auto *childDfa = childDfas[i];
    if (childDfa == nullptr) {
      continue;
    }
    childps[i]->findMatchingHelper(childDfa, callback);
  }
}

bool DynamicNode::isPlaceholder() const {
  return prefix_.empty() && wordsHere_.empty() && transitions_.empty();
}
}  // namespace z2kplus::backend::reverse_index::trie
