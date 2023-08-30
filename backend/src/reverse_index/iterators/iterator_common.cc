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

#include "z2kplus/backend/reverse_index/iterators/iterator_common.h"

#include <sys/mman.h>
#include <string>
#include <vector>
#include "kosak/coding/coding.h"

using kosak::coding::streamf;

namespace z2kplus::backend::reverse_index::iterators {

std::pair<wordRel_t, wordRel_t> IteratorContext::getFieldBoundsRel(const ZgramInfo &zgInfo,
    FieldTag fieldTag) const {
  // paranoia
  static_assert((int)FieldTag::sender == 0);
  static_assert((int)FieldTag::signature == 1);
  static_assert((int)FieldTag::instance == 2);
  static_assert((int)FieldTag::body == 3);
  static_assert((int)FieldTag::numTags == 4);

  const auto &z = zgInfo;

  static_assert(sizeof(wordRel_t) == 4);
  uint32 begin = z.startingWordOff().raw();
  uint32 end;
  switch (fieldTag) {
    case FieldTag::sender: {
      end = begin + z.senderWordLength();
      break;
    }
    case FieldTag::signature: {
      begin += z.senderWordLength();
      end = begin + z.signatureWordLength();
      break;
    }
    case FieldTag::instance: {
      begin += z.senderWordLength() + z.signatureWordLength();
      end = begin + z.instanceWordLength();
      break;
    }
    case FieldTag::body: {
      begin += z.senderWordLength() + z.signatureWordLength() + z.instanceWordLength();
      end = begin + z.bodyWordLength();
      break;
    }
    default: {
      crash("Unexpected tag %o", fieldTag);
    }
  }
  return maybeFlipPair<wordRel_t>(begin, end);
}

bool ZgramIteratorState::updateNextStart(const IteratorContext &ctx, zgramRel_t lowerBound,
    size_t capacity) {
  auto newNextStart = std::max(nextStart_, lowerBound);
  nextStart_ = newNextStart;
  if (capacity == 0) {
    return false;
  }
  auto bounds = ctx.getIndexZgBoundsRel();
  nextStart_ = std::max(bounds.first, nextStart_);
  return nextStart_ < bounds.second;
}

bool WordIteratorState::updateNextStart(const IteratorContext &ctx, wordRel_t lowerBound,
    size_t capacity) {
  auto newNextStart = std::max(nextStart_, lowerBound);
  nextStart_ = newNextStart;
  if (capacity == 0) {
    return false;
  }
  return nextStart_ < ctx.getIndexWordBoundsRel().second;
}

ZgramIterator::~ZgramIterator() = default;

// This method is biased towards finding the lower bound in the next few items
namespace {
template<typename Iterator, typename Key>
Iterator myLowerBound(Iterator begin, Iterator end, const Key &key) {
  // One common case is that the item to be found is very near
  size_t linearSearchLimit = 5;
  for (size_t i = 0; i < linearSearchLimit; ++i) {
    // want the first: *begin >= key
    // aka !(*begin < key)
    if (begin == end || !(*begin < key)) {
      return begin;
    }
    ++begin;
  }
  // Another common case is that the item will not be found
  // The element to be found is at the last item or before if end[-1] >= key
  // Otherwise it is later. So exit if !(end[-1] >= key)
  // aka: end[-1] < key
  if (begin == end || end[-1] < key) {
    return end;
  }
  return std::lower_bound(begin, end, key);
}
}  // namespace

ZgramStreamer::ZgramStreamer() = default;
ZgramStreamer::ZgramStreamer(const ZgramIterator *child, std::unique_ptr<ZgramIteratorState> &&childState) :
    child_(child), childState_(std::move(childState)), data_(std::make_unique<zgramRel_t[]>(bufferCapacity)),
    current_(nullptr), end_(nullptr) {
}
ZgramStreamer::ZgramStreamer(ZgramStreamer &&other) noexcept = default;
ZgramStreamer &ZgramStreamer::operator=(ZgramStreamer &&other) noexcept = default;
ZgramStreamer::~ZgramStreamer() = default;

bool ZgramStreamer::tryGetOrAdvance(const IteratorContext &ctx, zgramRel_t lowerBound,
    zgramRel_t *result) {
  while (true) {
    if (current_ == end_) {
      current_ = data_.get();
      auto size = child_->getMore(ctx, childState_.get(), lowerBound, data_.get(), bufferCapacity);
      end_ = current_ + size;
      if (size == 0) {
        return false;
      }
    }
    current_ = myLowerBound(current_, end_, lowerBound);
    if (current_ != end_) {
      *result = *current_;
      return true;
    }
  }
}

WordStreamer::WordStreamer() = default;
WordStreamer::WordStreamer(const WordIterator *child, std::unique_ptr<WordIteratorState> &&childState) :
    child_(child), childState_(std::move(childState)), data_(std::make_unique<wordRel_t[]>(bufferCapacity)),
    current_(nullptr), end_(nullptr) {
}

WordStreamer::WordStreamer(WordStreamer &&other) noexcept = default;
WordStreamer &WordStreamer::operator=(WordStreamer &&other) noexcept = default;
WordStreamer::~WordStreamer() = default;

bool WordStreamer::tryGetOrAdvance(const IteratorContext &ctx, wordRel_t lowerBound,
    wordRel_t *result) {
  while (true) {
    if (current_ == end_) {
      current_ = data_.get();
      auto size = child_->getMore(ctx, childState_.get(), lowerBound, data_.get(), bufferCapacity);
      end_ = current_ + size;
      if (size == 0) {
        return false;
      }
    }
    current_ = myLowerBound(current_, end_, lowerBound);
    if (current_ != end_) {
      *result = *current_;
      return true;
    }
  }
}
}  // namespace z2kplus::backend::reverse_index::iterators
