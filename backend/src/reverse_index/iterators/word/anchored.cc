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

#include "z2kplus/backend/reverse_index/iterators/word/anchored.h"

#include "z2kplus/backend/util/misc.h"

namespace z2kplus::backend::reverse_index::iterators {

using kosak::coding::streamf;

// Optimization:
// * A word that is not anchored on either side can be more simply handled.
std::unique_ptr<WordIterator> Anchored::create(std::unique_ptr<WordIterator> &&child, bool anchoredLeft,
  bool anchoredRight) {
  if (!anchoredLeft && !anchoredRight) {
    // No anchors -> NOP
    return std::move(child);
  }

  bool childAnchoredLeft, childAnchoredRight;
  std::unique_ptr<WordIterator> childChild;
  if (child->tryGetAnchorChild(&childChild, &childAnchoredLeft, &childAnchoredRight)) {
    return std::make_unique<Anchored>(Private(), std::move(childChild),
        anchoredLeft || childAnchoredLeft, anchoredRight || childAnchoredRight);
  }
  return std::make_unique<Anchored>(Private(), std::move(child), anchoredLeft, anchoredRight);
}

Anchored::Anchored(Private, std::unique_ptr<WordIterator> &&child, bool anchoredLeft,
    bool anchoredRight) : child_(std::move(child)), anchoredLeft_(anchoredLeft),
    anchoredRight_(anchoredRight) {}
Anchored::~Anchored() = default;

bool Anchored::tryGetAnchorChild(std::unique_ptr<WordIterator> *child, bool *anchoredLeft,
  bool *anchoredRight) {
  *child = std::move(child_);
  *anchoredLeft = anchoredLeft_;
  *anchoredRight = anchoredRight_;
  return true;
}

std::unique_ptr<WordIteratorState> Anchored::createState(const IteratorContext &ctx) const {
  // We have no state of our own.
  return child_->createState(ctx);
}

size_t Anchored::getMore(const IteratorContext &ctx, WordIteratorState *state, wordRel_t lowerBound,
    wordRel_t *result, size_t capacity) const {
  while (true) {
    auto childSize = child_->getMore(ctx, state, lowerBound, result, capacity);
    if (childSize == 0) {
      // exhausted.
      return 0;
    }
    auto newSize = applyFilter(ctx, state, result, childSize);
    if (newSize != 0) {
      return newSize;
    }
  }
}

size_t Anchored::applyFilter(const IteratorContext &ctx, WordIteratorState *state,
    wordRel_t *result, size_t size) const {
  // the src and dest buffers overlap
  auto *src = result;
  const auto *srcEnd = src + size;
  auto *dest = result;

  const auto &ci = ctx.ci();

  while (src != srcEnd) {
    auto wordOff = ctx.relToOff(*src);
    auto zgramOff = ci.getWordInfo(wordOff).zgramOff();
    const auto &zg = ci.getZgramInfo(zgramOff);
    auto target = zg.startingWordOff();

    // paranoia
    static_assert((int)FieldTag::sender == 0);
    static_assert((int)FieldTag::signature == 1);
    static_assert((int)FieldTag::instance == 2);
    static_assert((int)FieldTag::body == 3);
    static_assert((int)FieldTag::numTags == 4);

    auto check = [this, &target, wordOff](size_t width) {
      if (width == 0) {
        // TODO(kosak): I'm not sure this is even allowed. Doesn't hurt anything though.
        return false;
      }
      auto left = target;
      auto right = target.addRaw(width - 1);
      target = target.addRaw(width);

      return (!anchoredLeft_ || wordOff == left) &&
        (!anchoredRight_ || wordOff == right);
    };

    if (check(zg.senderWordLength()) ||
        check(zg.signatureWordLength()) ||
        check(zg.instanceWordLength()) ||
        check(zg.bodyWordLength())) {
      *dest++ = *src;
    }
    ++src;
  }
  return dest - result;
}

void Anchored::dump(std::ostream &s) const {
  streamf(s, "Anchor(%o%o%o)",
    anchoredLeft_ ? "left, " : "",
    anchoredRight_ ? "right, " : "",
    *child_);
}

}  // namespace z2kplus::backend::reverse_index::iterators
