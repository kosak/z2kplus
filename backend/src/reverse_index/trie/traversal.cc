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

#if 0
#include <string>
#include <utility>
#include <vector>
#include "z2kplus/backend/reverse_index/trie/dynamic_node.h"
#include "z2kplus/backend/reverse_index/trie/expanded_node.h"
#include "z2kplus/backend/reverse_index/trie/freezable_trie.h"
#include "z2kplus/backend/reverse_index/trie/frozen_node.h"
#include "z2kplus/backend/util/automaton/automaton.h"
#include "z2kplus/backend/util/misc.h"

using kosak::coding::FriendlyUtf8;
using kosak::coding::Hexer;
using z2kplus::backend::reverse_index::trie::ExpandedNode;
using z2kplus::backend::util::automaton::DFANode;

namespace z2kplus::backend::reverse_index::trie {

namespace {
void matchFrozenTrie(const FrozenNode *node, const DFANode *pattern,
  vector<pair<const uint32 *, const uint32 *>> *ranges);
void matchDynamicTrie(const DynamicNode &node, const DFANode *pattern,
  vector<pair<const uint32 *, const uint32 *>> *ranges);
}  // namespace

void findMatchingWords(const FreezableTrie &trie,
  const z2kplus::backend::util::automaton::FiniteAutomaton &dfa,
  std::vector<std::pair<const uint32*, const uint32*>> *ranges) {
  const auto *pattern = dfa.start();
  auto nh = trie.rootAsNodeHandle();
  const auto *frozen = nh.asFrozen();
  if (frozen != nullptr) {
    matchFrozenTrie(frozen, pattern, ranges);
    return;
  }
  const auto *dynamic = nh.asDynamic();
  if (dynamic != nullptr) {
    matchDynamicTrie(*nh.asDynamic(), pattern, ranges);
  }
}

namespace {
void matchFrozenTrie(const FrozenNode *node, const DFANode *pattern,
  vector<pair<const uint32 *, const uint32 *>> *ranges) {
  if (pattern->accepting() && node->endsHere()) {
    ExpandedNode exp(node);
    const auto &wh = exp.wordsHere();
    ranges->push_back(make_pair(&*wh.begin(), &*wh.end()));
  }
  // Trie iterator
  NodeHandle nh(node);
  auto tit = nh.getTransitionIterator();
  TransitionIterator::item_type tItem;
  if (!tit.tryGetNext(&tItem)) {
    return;
  }

  // Pattern iterator
  const auto &pTrans = pattern->transitions();
  auto pit = pTrans.begin();
  if (pit == pTrans.end()) {
    return;
  }

  while (true) {
    // March trie iterator forward until tFirst >= pattern range begin
    while ((unsigned char)tItem.first[0] < pit->begin()) {
      if (!tit.tryGetNext(&tItem)) {
        return;
      }
    }
    auto tFirst = (unsigned char)tItem.first[0];

    // Now march pattern iterator forward until tFirst < pattern range end
    while (tFirst >= pit->end()) {
      if (++pit == pTrans.end()) {
        return;
      }
    }

    // At this point. tFirst < pit->end()
    // But is it still >= pit->begin()?
    if (tFirst < pit->begin()) {
      // oops, overshot pit
      continue;
    }

    // In range, so recurse.
    auto newPattern = pit->node()->tryAdvance(tItem.first.substr(1));
    if (newPattern != nullptr) {
      matchFrozenTrie(tItem.second.asFrozen(), newPattern, ranges);
    }
    // Advance the trie.
    if (!tit.tryGetNext(&tItem)) {
      return;
    }
  }
}

void matchDynamicTrie(const DynamicNode &node, const DFANode *pattern,
  vector<pair<const uint32 *, const uint32 *>> *ranges) {
  if (pattern->accepting() && node.endsHere()) {
    const auto &wh = *node.wordsHere();
    ranges->push_back(make_pair(&*wh.begin(), &*wh.end()));
  }

  // Trie iterator
  const auto &tTrans = node.children();
  auto tit = tTrans.begin();
  if (tit == tTrans.end()) {
    return;
  }

  // Pattern iterator
  const auto &pTrans = pattern->transitions();
  auto pit = pTrans.begin();
  if (pit == pTrans.end()) {
    return;
  }

  while (true) {
    // March trie iterator forward until tFirst >= pattern range min
    while ((unsigned char)tit->first[0] < pit->begin()) {
      if (++tit == tTrans.end()) {
        return;
      }
    }
    auto tFirst = (unsigned char)tit->first[0];

    // Now march pattern iterator forward until tFirst <= pattern range max
    while (tFirst >= pit->end()) {
      if (++pit == pTrans.end()) {
        return;
      }
    }

    // At this point. tFirst < pit->end()
    // But is it still >= pit->begin()?
    if (tFirst < pit->begin()) {
      // oops, overshot pit
      continue;
    }

    // In range, so recurse.
    auto newPattern = pit->node()->tryAdvance(std::string_view(tit->first).substr(1));
    if (newPattern != nullptr) {
      matchDynamicTrie(tit->second, newPattern, ranges);
    }
    // Advance the trie
    if (++tit == tTrans.end()) {
      return;
    }
  }
}
}  // namespace

}  // namespace z2kplus::backend::reverse_index::trie
#endif
