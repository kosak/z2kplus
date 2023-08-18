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
template<typename Source, size_t Size>
class PrefixGrabber final : public TupleIterator<typename traits::TruncateTuple<Size, Source>::type> {
  typedef kosak::coding::FailFrame FailFrame;
public:
  typedef typename traits::TruncateTuple<Size, Source>::type result_t;
  explicit PrefixGrabber(TupleIterator<Source> *src) : src_(src) {}
  DISALLOW_COPY_AND_ASSIGN(PrefixGrabber);
  DEFINE_MOVE_COPY_AND_ASSIGN(PrefixGrabber);
  ~PrefixGrabber() = default;

  bool tryGetNext(std::optional<result_t> *result, const FailFrame &ff) final;
  void reset() final {
    src_->reset();
  }

private:
  TupleIterator<Source> *src_ = nullptr;
};

template<typename Source, size_t KeySize>
bool PrefixGrabber<Source, KeySize>::tryGetNext(std::optional<result_t> *result, const FailFrame &ff) {
  while (true) {
    std::optional<Source> temp;
    if (!src_->tryGetNext(&temp, ff.nest(KOSAK_CODING_HERE))) {
      // Error, so bail out.
      return false;
    }
    if (!temp.has_value()) {
      // Inner stream ended.
      result->reset();
      return true;
    }
    *result = result_t();
    copySegment<0, KeySize>(*temp, &**result);
    return true;
  }
}

template<size_t Size, typename Source>
inline auto makePrefixGrabber(TupleIterator<Source> *src) {
  return PrefixGrabber<Source, Size>(src);
}
}   // namespace z2kplus::backend::reverse_index::builder::tuple_iterators
