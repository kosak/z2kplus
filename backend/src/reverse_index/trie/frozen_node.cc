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

#include "z2kplus/backend/reverse_index/trie/frozen_node.h"

#include <string>
#include "kosak/coding/coding.h"
#include "kosak/coding/text/conversions.h"
#include "z2kplus/backend/util/misc.h"

namespace z2kplus::backend::reverse_index::trie {

using kosak::coding::FailFrame;
using kosak::coding::bit_cast;
using kosak::coding::Delegate;
using kosak::coding::Hexer;
using kosak::coding::streamf;
using kosak::coding::text::tryConvertUtf32ToUtf8;
using z2kplus::backend::util::automaton::DFANode;
using z2kplus::backend::util::RelativePtr;
using z2kplus::backend::util::automaton::FiniteAutomaton;

#define HERE KOSAK_CODING_HERE

namespace {
class FrozenNodeView {
  template<typename T>
  using RelativePtr = z2kplus::backend::util::RelativePtr<T>;
public:
  explicit FrozenNodeView(const FrozenNode *fn);

  bool tryFind(std::u32string_view probe,
      std::pair<const wordOff_t *, const wordOff_t *> *result) const;

  void findMatching(const DFANode *node,
      const Delegate<void, const wordOff_t *, const wordOff_t *> &callback) const;

  bool tryDump(std::ostream &s, std::string *debugReadable, const FailFrame &ff);

private:
  const FrozenNode *self_ = nullptr;
  std::u32string_view prefix_;
  const wordOff_t *wordsHere_ = nullptr;
  size_t numWordsHere_ = 0;
  std::u32string_view transitionKeys_;
  const RelativePtr<FrozenNode> *transitions_ = nullptr;
};
}  // namespace

bool FrozenNode::tryFind(std::u32string_view probe,
    std::pair<const wordOff_t *, const wordOff_t *> *result) const {
  FrozenNodeView fnv(this);
  return fnv.tryFind(probe, result);
}

void FrozenNode::findMatching(const FiniteAutomaton &dfa,
    const Delegate<void, const wordOff_t *, const wordOff_t *> &callback) const {
  FrozenNodeView fnv(this);
  fnv.findMatching(dfa.start(), callback);
}

bool FrozenNode::tryDump(std::ostream &s, std::string *debugReadable, const FailFrame &ff) const {
  FrozenNodeView fnv(this);
  return fnv.tryDump(s, debugReadable, ff.nest(HERE));
}

namespace {
FrozenNodeView::FrozenNodeView(const FrozenNode *fn) : self_(fn) {
  const auto *prefixBegin = fn->prefix_;
  const auto *prefixEnd = prefixBegin + fn->prefixSize_;
  const auto *wordsBegin = bit_cast<const wordOff_t*>(prefixEnd);
  const auto *wordsEnd = wordsBegin + fn->numWordsHere_;
  const auto *transitionKeysBegin = bit_cast<const char32_t*>(wordsEnd);
  const auto *transitionKeysEnd = transitionKeysBegin + fn->numTransitions_;
  auto paddingEnd = (reinterpret_cast<uintptr_t>(transitionKeysEnd) + 7) & ~uintptr_t(7);
  const auto *transitionsBegin = reinterpret_cast<RelativePtr<FrozenNode>*>(paddingEnd);

  prefix_ = std::u32string_view(prefixBegin, fn->prefixSize_);
  wordsHere_ = wordsBegin;
  numWordsHere_ = fn->numWordsHere_;
  transitionKeys_ = std::u32string_view(transitionKeysBegin, fn->numTransitions_);
  transitions_ = transitionsBegin;
}

bool FrozenNodeView::tryFind(std::u32string_view probe,
    std::pair<const wordOff_t *, const wordOff_t *> *result) const {
  if (probe.substr(0, prefix_.size()) != prefix_) {
    return false;
  }
  auto residual = probe.substr(prefix_.size());
  if (residual.empty()) {
    if (numWordsHere_ == 0) {
      return false;
    }
    result->first = wordsHere_;
    result->second = wordsHere_ + numWordsHere_;
    return true;
  }
  auto er = std::equal_range(transitionKeys_.begin(), transitionKeys_.end(), residual[0]);
  if (er.first == er.second) {
    return false;
  }
  auto index = er.first - transitionKeys_.begin();
  FrozenNodeView child(transitions_[index].get());
  return child.tryFind(residual.substr(1), result);
}

void FrozenNodeView::findMatching(const DFANode *dfaNode,
    const Delegate<void, const wordOff_t *, const wordOff_t *> &callback) const {
  const auto *dfaToUse = dfaNode->tryAdvance(prefix_);
  if (dfaToUse == nullptr) {
    return;
  }

  if (numWordsHere_ != 0 && dfaToUse->accepting()) {
    callback(wordsHere_, wordsHere_ + numWordsHere_);
  }

  if (transitionKeys_.empty()) {
    return;
  }

  const DFANode *childDfas[transitionKeys_.size()];
  dfaToUse->tryAdvanceMulti(transitionKeys_, childDfas);
  for (size_t i = 0; i < transitionKeys_.size(); ++i) {
    const auto *childDfa = childDfas[i];
    if (childDfa == nullptr) {
      continue;
    }
    FrozenNodeView child(transitions_[i].get());
    child.findMatching(childDfa, callback);
  }
}

bool FrozenNodeView::tryDump(std::ostream &s, std::string *debugReadable,
    const FailFrame &ff) {
  auto saveSize = debugReadable->size();
  if (!tryConvertUtf32ToUtf8(prefix_, debugReadable, ff.nest(HERE))) {
    return false;
  }
  std::string_view prefixSv(debugReadable->data() + saveSize, debugReadable->size() - saveSize);
  streamf(s, "0x%o: pfx=%o (%o) nw=%o %o", Hexer((uintptr_t)self_), prefixSv, *debugReadable,
      numWordsHere_, kosak::coding::dump(wordsHere_, wordsHere_ + numWordsHere_));

  for (size_t i = 0; i < transitionKeys_.size(); ++i) {
    auto key = transitionKeys_[i];
    const auto *transition = transitions_[i].get();
    auto innerSave = debugReadable->size();
    std::u32string_view keySv(&key, 1);
    if (!tryConvertUtf32ToUtf8(keySv, debugReadable, ff.nest(HERE))) {
      return false;
    }
    std::string_view tranSv(debugReadable->data() + innerSave, debugReadable->size() - innerSave);
    streamf(s, "\n%o - 0x%o (%o)", tranSv, Hexer((uintptr_t)transition), *debugReadable);
    debugReadable->erase(innerSave);
  }
  for (size_t i = 0; i < transitionKeys_.size(); ++i) {
    auto key = transitionKeys_[i];
    const auto *transition = transitions_[i].get();
    auto innerSave = debugReadable->size();
    std::u32string_view keySv(&key, 1);
    if (!tryConvertUtf32ToUtf8(keySv, debugReadable, ff.nest(HERE))) {
      return false;
    }
    s << '\n';
    if (!transition->tryDump(s, debugReadable, ff.nest(HERE))) {
      return false;
    }
    debugReadable->erase(innerSave);
  }
  debugReadable->erase(saveSize);
  return true;
}
}  // namespace
}  // namespace z2kplus::backend::reverse_index::trie
