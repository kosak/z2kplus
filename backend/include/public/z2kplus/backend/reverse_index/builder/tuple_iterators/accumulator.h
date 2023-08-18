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
class Accumulator final : public TupleIterator<Tuple> {
  typedef kosak::coding::FailFrame FailFrame;
public:
  explicit Accumulator(TupleIterator<Tuple> *src) : src_(src) {}

  ~Accumulator() = default;

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
bool Accumulator<Tuple, KeySize>::tryGetNext(std::optional<Tuple> *result, const FailFrame &ff) {
  while (true) {
    using std::swap;
    // Optimistically load prev_ into the result
    *result = std::move(prev_);
    // We're done with prev_ so let's call it "next" from now on.
    auto &next = prev_;
    if (!src_->tryGetNext(&next, ff.nest(KOSAK_CODING_HERE))) {
      // Error, so bail out.
      return false;
    }
    if (!next.has_value()) {
      // Inner stream ended, so we are returning our last item (in *result), which is either indeed
      // one more item or empty forevermore.
      return true;
    }
    if (!result->has_value()) {
      // Inner stream didn't end, but prior value (the one we were about to return) was empty,
      // so this must be the first time.
      continue;
    }
    if (findFirstDifference<0, KeySize>(**result, *next) != KeySize) {
      // Key changed, so you can return the accumulated value.
      return true;
    }
    // Merge the value
    std::get<KeySize>(*next) += std::get<KeySize>(**result);
  }
}

template<size_t KeySize, typename Tuple>
inline auto makeAccumulator(TupleIterator<Tuple> *src) {
  return Accumulator<Tuple, KeySize>(src);
}
}   // namespace z2kplus::backend::reverse_index::builder::tuple_iterators
