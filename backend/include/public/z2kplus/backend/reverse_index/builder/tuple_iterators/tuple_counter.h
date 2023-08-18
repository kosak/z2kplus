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

#pragma once

#include <ostream>
#include "kosak/coding/coding.h"
#include "kosak/coding/memory/mapped_file.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/memory/buffered_writer.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/reverse_index/builder/common.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/iterator_base.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/util.h"
#include "z2kplus/backend/util/frozen/frozen_string_pool.h"

namespace z2kplus::backend::reverse_index::builder::tuple_iterators {
namespace internal {

class DiffIteratorBase {
protected:
  typedef kosak::coding::FailFrame FailFrame;
  template<typename T>
  using TupleIterator = z2kplus::backend::reverse_index::builder::tuple_iterators::TupleIterator<T>;

public:
  virtual ~DiffIteratorBase();
  virtual bool tryGetNext(std::optional<size_t> *result, const FailFrame &ff) = 0;
};

template<typename Tuple>
class DiffIterator final : public DiffIteratorBase {
public:
  explicit DiffIterator(TupleIterator<Tuple> *iter) : iter_(iter) {}
  ~DiffIterator() final = default;
  bool tryGetNext(std::optional<size_t> *result, const FailFrame &ff) final;

private:
  // Does not own.
  TupleIterator<Tuple> *iter_ = nullptr;
  std::optional<Tuple> item_;
};
}  // namespace internal

class TupleCounts {
  typedef kosak::coding::FailFrame FailFrame;
  template<typename T>
  using TupleIterator = z2kplus::backend::reverse_index::builder::tuple_iterators::TupleIterator<T>;
  template<typename T>
  using MappedFile = kosak::coding::memory::MappedFile<T>;

public:
  template<typename Tuple>
  static bool tryCreate(const std::string &filename, TupleIterator<Tuple> *iter, size_t treeHeight, TupleCounts *result,
      const FailFrame &ff) {
    internal::DiffIterator<Tuple> diffIterator(iter);
    return tryCreateHelper(filename, &diffIterator, treeHeight, result, ff.nest(KOSAK_CODING_HERE));
  }

  TupleCounts();
  DISALLOW_COPY_AND_ASSIGN(TupleCounts);
  DECLARE_MOVE_COPY_AND_ASSIGN(TupleCounts);
  ~TupleCounts();

  bool tryGetNext(std::optional<uint64_t> *result, const FailFrame &ff);

  void reset() {
    current_ = mf_.get();
  }

private:
  static bool tryCreateHelper(const std::string &filename, internal::DiffIteratorBase *iter,
      size_t treeHeight, TupleCounts *result, const FailFrame &ff);

  TupleCounts(MappedFile<uint64_t> mf, const uint64_t *current, const uint64_t *end);

  MappedFile<uint64_t> mf_;
  const uint64_t *current_ = nullptr;
  const uint64_t *end_ = nullptr;
};

namespace internal {
template<typename Tuple>
bool DiffIterator<Tuple>::tryGetNext(std::optional<size_t> *result, const FailFrame &ff) {
  // dodge gcc bug
  // std::optional<Tuple> prev = std::move(item_);
  std::optional<Tuple> prev;
  if (item_.has_value()) {
    prev = std::move(item_);
  }
  // If error, then return an error indication.
  if (!iter_->tryGetNext(&item_, ff.nest(KOSAK_CODING_HERE))) {
    return false;
  }
  // Otherwise, if end of stream, return an end of stream indication
  if (!item_.has_value()) {
    result->reset();
    return true;
  }
  // Otherwise, if first time, return a sensible value for "first time"
  if (!prev.has_value()) {
    *result = 0;
    return true;
  }
  // Otherwise, return the result of the comparison
  constexpr auto size = std::tuple_size_v<Tuple>;
  *result = findFirstDifference<0, size>(*prev, *item_);
  return true;
}
}  // namespace internal
}   // namespace z2kplus::backend::reverse_index::builder::tuple_iterators
