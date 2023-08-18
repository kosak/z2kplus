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

#include <optional>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/text/misc.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/iterator_base.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/util.h"

namespace z2kplus::backend::reverse_index::builder::tuple_iterators {
namespace internal {
template<typename T>
struct ExtenderHelper {};

template<typename ...Args>
struct ExtenderHelper<std::tuple<Args...>> {
  typedef typename std::tuple<Args..., size_t> type;
};

template<size_t KeySize, typename Tuple>
struct Extender {
  typedef typename traits::TruncateTuple<KeySize, Tuple>::type truncated_t;
  typedef typename ExtenderHelper<truncated_t>::type type;
};
}  // namespace internal

template<typename Tuple, size_t KeySize>
class TupleCounter final : public TupleIterator<typename internal::Extender<KeySize, Tuple>::type> {
  typedef kosak::coding::FailFrame FailFrame;
public:
  typedef typename internal::Extender<KeySize, Tuple>::type extended_t;

  explicit TupleCounter(TupleIterator<Tuple> *src) : src_(src) {}
  DISALLOW_COPY_AND_ASSIGN(TupleCounter);
  DEFINE_MOVE_COPY_AND_ASSIGN(TupleCounter);
  ~TupleCounter() = default;

  bool tryGetNext(std::optional<extended_t> *result, const FailFrame &ff) final;

  void reset() final {
    src_->reset();
    prev_.reset();
  }

private:
  TupleIterator<Tuple> *src_ = nullptr;
  std::optional<Tuple> prev_;
};

template<typename Tuple, size_t KeySize>
bool TupleCounter<Tuple, KeySize>::tryGetNext(std::optional<extended_t> *result, const FailFrame &ff) {
  if (!prev_.has_value()) {
    if (!src_->tryGetNext(&prev_, ff.nest(KOSAK_CODING_HERE))) {
      return false;
    }
    if (!prev_.has_value()) {
      result->reset();
      return true;
    }
  }

  *result = extended_t();
  copySegment<0, KeySize>(*prev_, &**result);
  std::get<KeySize>(**result) = 1;

  while (true) {
    if (!src_->tryGetNext(&prev_, ff.nest(KOSAK_CODING_HERE))) {
      // Error, so bail out.
      return false;
    }
    if (!prev_.has_value()) {
      return true;
    }
    if (findFirstDifference<0, KeySize>(*prev_, **result) != KeySize) {
      // Key changed, so you can return accumualated count.
      return true;
    }

    // Key did not change, so increment count
    ++std::get<KeySize>(**result);
  }
}

template<size_t KeySize, typename Tuple>
inline auto makeCounter(TupleIterator<Tuple> *src) {
  return TupleCounter<Tuple, KeySize>(src);
}
}   // namespace z2kplus::backend::reverse_index::builder::tuple_iterators
