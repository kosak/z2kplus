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

#include "z2kplus/backend/reverse_index/iterators/boundary/word_adaptor.h"

#include "z2kplus/backend/reverse_index/iterators/zgram/popornot.h"

namespace z2kplus::backend::reverse_index::iterators::boundary {

using kosak::coding::streamf;
using z2kplus::backend::reverse_index::iterators::zgram::PopOrNot;

namespace {
class MyState final : public ZgramIteratorState {
public:
  explicit MyState(std::unique_ptr<WordIteratorState> &&childState);
  ~MyState() final;

  size_t getMore(const IteratorContext &ctx, const WordIterator *child, zgramRel_t lowerBound,
      zgramRel_t *result, size_t capacity);

private:
  std::unique_ptr<WordIteratorState> childState_;
  // For simplicity we always have a buffer large enough to serve our largest request, and
  // we always give back all of our results.
  std::unique_ptr<wordRel_t[]> source_;
  size_t sourceCapacity_ = 0;
};
}  // namespace

// Optimization:
// * If the child matches any word (for some set of fields), then we can be a Populated operator
// for that same set of fields. Populated is more efficient because it doesn't bother iterating
// over words.
std::unique_ptr<ZgramIterator> WordAdaptor::create(std::unique_ptr<WordIterator> &&child) {
  FieldMask fieldMask;
  if (child->matchesAnyWord(&fieldMask)) {
    return PopOrNot::create(fieldMask, FieldMask::none);
  }
  return std::make_unique<WordAdaptor>(Private(), std::move(child));
}

WordAdaptor::WordAdaptor(Private, std::unique_ptr<WordIterator> child) : child_(std::move(child)) {}
WordAdaptor::~WordAdaptor() = default;

std::unique_ptr<ZgramIteratorState> WordAdaptor::createState(const IteratorContext &ctx) const {
  auto childState = child_->createState(ctx);
  return std::make_unique<MyState>(std::move(childState));
}

size_t WordAdaptor::getMore(const IteratorContext &ctx, ZgramIteratorState *state,
    zgramRel_t lowerBound, zgramRel_t *result, size_t capacity) const {
  auto *ms = dynamic_cast<MyState *>(state);
  return ms->getMore(ctx, child_.get(), lowerBound, result, capacity);
}

void WordAdaptor::dump(std::ostream &s) const {
  streamf(s, "Adapt(%o)", *child_);
}

MyState::MyState(std::unique_ptr<WordIteratorState> &&childState) :
    childState_(std::move(childState)) {
  // Buffer starts out null, and of size 0
}
MyState::~MyState() = default;

size_t MyState::getMore(const IteratorContext &ctx, const WordIterator *child,
    zgramRel_t lowerBound, zgramRel_t *result, size_t capacity) {
  if (!updateNextStart(ctx, lowerBound, capacity)) {
    return 0;
  }
  if (sourceCapacity_ < capacity) {
    source_ = std::make_unique<wordRel_t[]>(capacity);
    sourceCapacity_ = capacity;
  }

  const auto &ci = ctx.ci();

  // Regardless of how big our buffer happens to be, we only use 'capacity' entries of it. That way
  // we can pass the whole result back to our caller without having to do the work of keeping track
  // of residual entries for next time.
  auto wbLowerBound = ctx.getWordBoundsRel(ci.getZgramInfo(ctx.relToOff(nextStart_))).first;
  auto childSize = child->getMore(ctx, childState_.get(), wbLowerBound, source_.get(), capacity);
  if (childSize == 0) {
    return 0;
  }

  const auto *srcBegin = source_.get();
  const auto *srcEnd = srcBegin + childSize;

  auto *dest = result;
  passert(childSize <= capacity);  // So we don't need to check dest size in this loop
  for (const auto *src = srcBegin; src != srcEnd; ++src) {
    auto wordRel = *src;
    auto zgRel = ctx.offToRel(ci.getWordInfo(ctx.relToOff(wordRel)).zgramOff());
    // filter duplicates
    if (dest == result || zgRel > *(dest - 1)) {
      *dest++ = zgRel;
    }
  }
  auto resultSize = dest - result;
  passert(resultSize != 0);
  nextStart_ = (dest - 1)->addRaw(1);
  return resultSize;
}
}  // namespace z2kplus::backend::reverse_index::iterators::boundary

