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

#include "z2kplus/backend/reverse_index/iterators/word/pattern.h"

#include <memory>
#include <utility>
#include <vector>
#include "kosak/coding/merger.h"
#include "z2kplus/backend/reverse_index/iterators/word/any_word.h"
#include "z2kplus/backend/reverse_index/index/consolidated_index.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/util/automaton/automaton.h"

namespace z2kplus::backend::reverse_index::iterators::word {

using kosak::coding::streamf;
using kosak::coding::merger::Merger;
using z2kplus::backend::reverse_index::index::ConsolidatedIndex;
using z2kplus::backend::util::automaton::FiniteAutomaton;

namespace {
struct MyState : public WordIteratorState {
  size_t getMore(const IteratorContext &ctx, FieldMask fieldMask,
      const FiniteAutomaton &dfa, wordRel_t *result, size_t capacity);
};
}  // namespace

std::unique_ptr<WordIterator> Pattern::create(FiniteAutomaton &&dfa, FieldMask fieldMask) {
  if (dfa.start()->acceptsEverything()) {
    return AnyWord::create(fieldMask);
  }
  return std::make_unique<Pattern>(Private(), std::move(dfa), fieldMask);
}

Pattern::Pattern(Private, FiniteAutomaton &&dfa, FieldMask fieldMask) : dfa_(std::move(dfa)),
  fieldMask_(fieldMask) {}

Pattern::~Pattern() = default;

namespace {
class MyCallback final {
public:
  MyCallback(const IteratorContext &ctx, FieldMask fieldMask, wordOff_t nextStartOff,
      wordRel_t *buffer, size_t capacity);
  ~MyCallback() = default;

  void operator()(const wordOff_t *begin, const wordOff_t *end);
  wordRel_t finish();

  size_t size() const { return size_; }

private:
  void appendToBuffer(const wordOff_t *current, const wordOff_t *endToUse,
      int adjustment, int delta);
  void updateHeap(const wordOff_t *current, const wordOff_t *endToUse,
      int adjustment, int delta);

  const IteratorContext &ctx_;
  FieldMask fieldMask_ = FieldMask::none;
  wordOff_t nextStartOff_;

  wordRel_t *buffer_ = nullptr;
  size_t capacity_ = 0;
  size_t size_ = 0;

  wordRel_t maximumWordRelSeen_;
};
}  // namespace

std::unique_ptr<WordIteratorState> Pattern::createState(const IteratorContext &/*ctx*/) const {
  return std::make_unique<MyState>();
}

// This is tricky because the pattern might match multiple words. We need to interleave all the
// wordlists and do chunking if it turns out that the wordlists are very large. The trick we use
// is to fill the caller's buffer sequentially until it is full, and then heapify it and from then
// on keep the smallest 'capacity' items.
size_t Pattern::getMore(const IteratorContext &ctx, WordIteratorState *state, wordRel_t lowerBound,
    wordRel_t *result, size_t capacity) const {
  if (fieldMask_ == FieldMask::none) {
    return 0;
  }
  auto *ms = dynamic_cast<MyState*>(state);
  if (!ms->updateNextStart(ctx, lowerBound, capacity)) {
    return 0;
  }
  return ms->getMore(ctx, fieldMask_, dfa_, result, capacity);
}

void Pattern::dump(std::ostream &s) const {
  streamf(s, "Pattern(%o, %o)", fieldMask_, dfa_.description());
}

namespace {
size_t MyState::getMore(const IteratorContext &ctx, FieldMask fieldMask,
    const FiniteAutomaton &dfa, wordRel_t *result, size_t capacity) {
  auto nextStartOff = ctx.relToOff(nextStart_);
  MyCallback cb(ctx, fieldMask, nextStartOff, result, capacity);
  ctx.ci().findMatching(dfa, &cb);
  auto newValue = cb.finish();
  nextStart_ = newValue;
  return cb.size();
}

MyCallback::MyCallback(const IteratorContext &ctx, FieldMask fieldMask,
    wordOff_t nextStartOff, wordRel_t *buffer, size_t capacity) : ctx_(ctx), fieldMask_(fieldMask),
    nextStartOff_(nextStartOff), buffer_(buffer), capacity_(capacity), size_(0) {}

void MyCallback::operator()(const wordOff_t *begin, const wordOff_t *end) {
  if (begin == end) {
    return;
  }

  // We might need this value if the filter passes no items.
  maximumWordRelSeen_ = std::max(maximumWordRelSeen_, ctx_.offToRel(end[-1]));

  // Normalize
  const wordOff_t *current;
  const wordOff_t *endToUse;
  int adjustment;
  int delta;
  if (ctx_.forward()) {
    current = std::lower_bound(begin, end, nextStartOff_);
    endToUse = end;
    adjustment = 0;
    delta = 1;
  } else {
    current = std::upper_bound(begin, end, nextStartOff_);
    endToUse = begin;
    adjustment = -1;
    delta = -1;
  }

  if (size_ != capacity_) {
    // Buffer isn't full yet, so append to it.
    appendToBuffer(current, endToUse, adjustment, delta);
  } else {
    // Buffer is full, so it has already been made into a heap. Update the heap with the
    // remaining items.
    updateHeap(current, endToUse, adjustment, delta);
  }
}

void MyCallback::appendToBuffer(const wordOff_t *current, const wordOff_t *endToUse,
    int adjustment, int delta) {
  auto *dest = buffer_ + size_;
  auto *destEnd = buffer_ + capacity_;
  const auto &ci = ctx_.ci();
  while (current != endToUse) {
    auto wordOff = current[adjustment];
    current += delta;
    auto fieldTag = ci.getWordInfo(wordOff).fieldTag();
    if (IteratorUtils::MaskContains(fieldMask_, fieldTag)) {
      auto wordRel = ctx_.offToRel(wordOff);
      *dest++ = wordRel;
      ++size_;
      if (dest == destEnd) {
        // Buffer is full! Transition to heap mode
        std::make_heap(buffer_, destEnd);
        updateHeap(current, endToUse, adjustment, delta);
        return;
      }
    }
  }
}

void MyCallback::updateHeap(const wordOff_t *current, const wordOff_t *endToUse,
    int adjustment, int delta) {
  auto *destEnd = buffer_ + capacity_;
  const auto &ci = ctx_.ci();
  while (current != endToUse) {
    auto wordOff = current[adjustment];
    current += delta;
    auto fieldTag = ci.getWordInfo(wordOff).fieldTag();
    if (IteratorUtils::MaskContains(fieldMask_, fieldTag)) {
      auto wordRel = ctx_.offToRel(wordOff);
      if (wordRel > buffer_[0]) {
        // max value in heap is at buffer[0]. Later items in this iteration will only be higher.
        // So we can just exit now.
        return;
      }
      // I think we could do this more efficiently as like buffer_[0] = wordRel, followed by a
      // fix_heap operation.
      std::pop_heap(buffer_, destEnd);
      destEnd[-1] = wordRel;
      std::push_heap(buffer_, destEnd);
    }
  }
}

wordRel_t MyCallback::finish() {
  std::sort(buffer_, buffer_ + size_);
  auto which = size_ > 0 ? buffer_[size_ - 1] : maximumWordRelSeen_;
  return which.addRaw(1);
}
}  // namespace
}  // namespace z2kplus::backend::reverse_index::iterators
