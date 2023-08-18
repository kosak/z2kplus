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

#include "z2kplus/backend/reverse_index/builder/common.h"
#include "kosak/coding/memory/mapped_file.h"

using kosak::coding::bit_cast;
using kosak::coding::FailFrame;
using kosak::coding::memory::MappedFile;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::reverse_index::builder {
RecordIterator::RecordIterator(MappedFile<char> &&mf) : mf_(std::move(mf)),
    remaining_(mf_.get(), mf_.byteSize()) {}
RecordIterator::~RecordIterator() = default;

bool RecordIterator::getNext(std::string_view *result) {
  if (remaining_.empty()) {
    return false;
  }

  auto lineIndex = remaining_.find_first_of(defaultRecordSeparator);
  if (lineIndex == std::string_view::npos) {
    *result = remaining_;
    remaining_ = "";
  } else {
    *result = remaining_.substr(0, lineIndex);
    remaining_ = remaining_.substr(lineIndex + 1);
  }
  return true;
}

void RecordIterator::reset() {
  remaining_ = std::string_view(mf_.get(), mf_.byteSize());
}

SimpleAllocator::SimpleAllocator(char *start, size_t capacity, size_t initialAlignment) :
    start_(start), capacity_(capacity), initialAlignment_(initialAlignment), offset_(0) {
  passert((initialAlignment_ & (initialAlignment_ - 1)) == 0, initialAlignment_);
}

bool SimpleAllocator::tryAllocate(size_t size, size_t alignment, char **result, const FailFrame &ff) {
  if (!tryAlign(alignment, ff.nest(HERE))) {
    return false;
  }
  *result = data();
  return tryAdvance(size, ff.nest(HERE));
}

bool SimpleAllocator::tryAlign(size_t alignment, const FailFrame &ff) {
  if (alignment > initialAlignment_) {
    return ff.failf(HERE, "Can't provide an alignment %o wider than initial alignment %o", alignment,
        initialAlignment_);
  }
  size_t mask = alignment - 1;
  if ((alignment & mask) != 0) {
    return ff.failf(HERE, "Alignment %o is not a power of 2", alignment);
  }
  if ((offset_ & mask) != 0) {
    auto numZeroes = alignment - (offset_ & mask);
    auto *dest = data();
    if (!tryAdvance(numZeroes, ff.nest(HERE))) {
      return false;
    }
    memset(dest, 0, numZeroes);
  }
  return true;
}

bool SimpleAllocator::tryAdvance(size_t size, const FailFrame &ff) {
  auto remaining = capacity_ - offset_;
  if (size > remaining) {
    return ff.failf(HERE, "Request %o exceeds remaining capacity %o", size, remaining);
  }
  offset_ += size;
  return true;
}
}  // namespace z2kplus::backend::reverse_index::builder
