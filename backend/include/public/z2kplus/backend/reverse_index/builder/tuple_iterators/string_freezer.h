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
#include "z2kplus/backend/util/frozen/frozen_string_pool.h"

namespace z2kplus::backend::reverse_index::builder::tuple_iterators {
template<typename Tuple, typename FrozenTuple>
class StringFreezer final : public TupleIterator<FrozenTuple> {
  typedef z2kplus::backend::util::frozen::FrozenStringPool FrozenStringPool;
  typedef kosak::coding::FailFrame FailFrame;
public:
  StringFreezer(TupleIterator<Tuple> *src, const FrozenStringPool *stringPool) : src_(src),
      stringPool_(stringPool) {}
  ~StringFreezer() = default;

  bool tryGetNext(std::optional<FrozenTuple> *result, const FailFrame &ff) final;
  void reset() final {
    src_->reset();
  }

private:
  // Does not own.
  TupleIterator<Tuple> *src_ = nullptr;
  // Does not own.
  const FrozenStringPool *stringPool_ = nullptr;
};

template<typename Tuple, typename FrozenTuple>
bool StringFreezer<Tuple, FrozenTuple>::tryGetNext(std::optional<FrozenTuple> *result,
    const FailFrame &ff) {
  while (true) {
    std::optional<Tuple> temp;
    if (!src_->tryGetNext(&temp, ff.nest(KOSAK_CODING_HERE))) {
      // Error, so bail out.
      return false;
    }
    if (!temp.has_value()) {
      // Inner stream ended.
      result->reset();
      return true;
    }
    *result = FrozenTuple();
    if (!tryFreezeTupleRecurse<0>(*temp, &**result, stringPool_)) {
      return ff.failf(KOSAK_CODING_HERE, "Couldn't freeze %o", *temp);
    }
    return true;
  }
}
template<typename Tuple>
inline auto makeStringFreezer(TupleIterator<Tuple> *src,
    const z2kplus::backend::util::frozen::FrozenStringPool *stringPool) {
  return StringFreezer<Tuple, typename traits::FreezeDimensions<Tuple>::type>(src, stringPool);
}
}   // namespace z2kplus::backend::reverse_index::builder::tuple_iterators
