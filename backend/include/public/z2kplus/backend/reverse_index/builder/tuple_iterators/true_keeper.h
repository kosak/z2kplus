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
template<typename Tuple, size_t FlagPosition>
class TrueKeeper final : public TupleIterator<Tuple> {
  typedef kosak::coding::FailFrame FailFrame;
public:
  explicit TrueKeeper(TupleIterator<Tuple> *src) : src_(src) {}

  ~TrueKeeper() = default;

  bool tryGetNext(std::optional<Tuple> *result, const FailFrame &ff) final;
  void reset() final {
    src_->reset();
  }

private:
  TupleIterator<Tuple> *src_ = nullptr;
};

template<typename Tuple, size_t FlagPosition>
bool TrueKeeper<Tuple, FlagPosition>::tryGetNext(std::optional<Tuple> *result, const FailFrame &ff) {
  while (true) {
    if (!src_->tryGetNext(result, ff.nest(KOSAK_CODING_HERE))) {
      // Error, so bail out.
      return false;
    }
    if (!result->has_value()) {
      // Inner stream ended.
      return true;
    }
    // Only return tuples where the flag is true.
    if (std::get<FlagPosition>(**result)) {
      return true;
    }
  }
}

template<size_t FlagPosition, typename Tuple>
inline auto makeTrueKeeper(TupleIterator<Tuple> *src) {
  return TrueKeeper<Tuple, FlagPosition>(src);
}
}   // namespace z2kplus::backend::reverse_index::builder::tuple_iterators
