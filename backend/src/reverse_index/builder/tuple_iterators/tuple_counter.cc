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

#include "z2kplus/backend/reverse_index/builder/tuple_iterators/tuple_counter.h"

#include "kosak/coding/coding.h"
#include "kosak/coding/memory/buffered_writer.h"
#include "kosak/coding/unix.h"

using kosak::coding::stringf;
using kosak::coding::FailFrame;
using kosak::coding::memory::BufferedWriter;
using kosak::coding::nsunix::FileCloser;

namespace nsunix = kosak::coding::nsunix;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::reverse_index::builder::tuple_iterators {
namespace {
class DiffProcessor {
public:
  DiffProcessor(size_t treeHeight, internal::DiffIteratorBase *iter, std::optional<size_t> item,
      uint64_t *current, const uint64_t *end) : treeHeight_(treeHeight), iter_(iter), item_(item),
      current_(current), end_(end) {}

  bool tryRecurse(size_t level, const FailFrame &ff);

  uint64_t *current() const { return current_; }

private:
  // Number of collections in the tuple (aka height of the tree).
  size_t treeHeight_ = 0;
  internal::DiffIteratorBase *iter_ = nullptr;
  std::optional<size_t> item_;
  uint64_t *current_ = nullptr;
  const uint64_t *end_ = nullptr;
};
};  // namespace

TupleCounts::TupleCounts() = default;
TupleCounts::TupleCounts(MappedFile<uint64_t> mf, const uint64_t *current, const uint64_t *end) :
    mf_(std::move(mf)), current_(current), end_(end) {}
TupleCounts::TupleCounts(TupleCounts &&) noexcept = default;
TupleCounts &TupleCounts::operator=(TupleCounts &&) noexcept = default;
TupleCounts::~TupleCounts() = default;

bool TupleCounts::tryGetNext(std::optional<uint64_t> *result, const FailFrame &/*ff*/) {
  if (current_ == end_) {
    result->reset();
    return true;
  }

  *result = *current_++;
  return true;
}

bool TupleCounts::tryCreateHelper(const std::string &filename, internal::DiffIteratorBase *iter, size_t treeHeight,
    TupleCounts *result, const FailFrame &ff) {
  const auto hyperGenerousSize = size_t(1) << 36;  // 64G
  FileCloser fc;
  MappedFile<uint64_t> mf;
  std::optional<size_t> item;
  if (!nsunix::tryMakeFileOfSize(filename, 0644, hyperGenerousSize, ff.nest(HERE)) ||
      !mf.tryMap(filename, true, ff.nest(HERE)) ||
      !iter->tryGetNext(&item, ff.nest(HERE))) {
    return false;
  }

  auto *current = mf.get();
  const auto *end = current + (mf.byteSize() / sizeof(uint64_t));

  if (!item.has_value()) {
    // Special case for empty sequence
    *current++ = 0;
  } else {
    DiffProcessor dp(treeHeight, iter, item, current, end);
    if (!dp.tryRecurse(0, ff.nest(HERE))) {
      return false;
    }
    current = dp.current();
  }

  auto numElements = current - mf.get();
  auto finalSize = numElements * sizeof(*current);
  if (!mf.tryUnmap(ff.nest(HERE)) ||
      !nsunix::tryTruncate(filename, finalSize, ff.nest(HERE)) ||
      !mf.tryMap(filename, false, ff.nest(HERE))) {
    return false;
  }
  const auto *finalBegin = mf.get();
  const auto *finalEnd = finalBegin + numElements;
  *result = TupleCounts(std::move(mf), finalBegin, finalEnd);
  return true;
}

bool DiffProcessor::tryRecurse(size_t level, const FailFrame &ff) {
  if (level == treeHeight_) {
    return iter_->tryGetNext(&item_, ff.nest(HERE));
  }
  if (current_ == end_) {
    return ff.failf(HERE, "Logic error: Impossible: enormous file filled up");
  }
  // Allocate a slot for a counter and initialize it to 1
  auto *myCounter = current_++;
  *myCounter = 1;

  while (true) {
    if (!tryRecurse(level + 1, ff.nest(HERE))) {
      return false;
    }
    if (!item_.has_value() || *item_ < level) {
      return true;
    }
    // *item_ is probably == level, but if we're a multiset (no multisets currently implemented in the system)
    // and the the next diff is in the value part, then *item_ > level. That's ok though.
    ++*myCounter;
  }
}
namespace internal {
DiffIteratorBase::~DiffIteratorBase() = default;
}  // namespace internal
}  // namespace z2kplus::backend::reverse_index::builder::tuple_iterators
