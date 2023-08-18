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

#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/text/misc.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/iterator_base.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/util.h"

namespace z2kplus::backend::reverse_index::builder::tuple_iterators {
template<typename Tuple, size_t KeySize>
class RunningSum final : public TupleIterator<Tuple> {
  typedef kosak::coding::FailFrame FailFrame;
public:
  explicit RunningSum(TupleIterator<Tuple> *src) : src_(src) {}

  ~RunningSum() = default;

  bool tryGetNext(std::optional<Tuple> *result, const FailFrame &ff) final;

  void reset() final {
    src_->reset();
    prev_.reset();
  }

private:
  TupleIterator<Tuple> *src_ = nullptr;
  std::optional<Tuple> prev_;
};

template<typename Tuple, size_t KeySize>
bool RunningSum<Tuple, KeySize>::tryGetNext(std::optional<Tuple> *result, const FailFrame &ff) {
  if (!prev_.has_value()) {
    if (!src_->tryGetNext(&prev_, ff.nest(KOSAK_CODING_HERE))) {
      // Error, so bail out.
      return false;
    }
    if (!prev_.has_value()) {
      // Inner stream is empty.
      result->reset();
      return true;
    }
  }
  while (true) {
    // Optimistically load prev_ into the result
    *result = std::move(prev_);
    if (!src_->tryGetNext(&prev_, ff.nest(KOSAK_CODING_HERE))) {
      // Error, so bail out.
      return false;
    }
    if (!prev_.has_value()) {
      // Inner stream ended, so we are returning our last item (in *result), which is either indeed
      // one more item or empty forevermore.
      return true;
    }
    if (findFirstDifference<0, KeySize>(**result, *prev_) == KeySize) {
    // If the key is the same, update the next value with the sum from this value
      std::get<KeySize>(*prev_) += std::get<KeySize>(**result);
    }
    return true;
  }
}

template<size_t KeySize, typename Tuple>
inline auto makeRunningSum(TupleIterator<Tuple> *src) {
  return RunningSum<Tuple, KeySize>(src);
}
}   // namespace z2kplus::backend::reverse_index::builder::tuple_iterators
