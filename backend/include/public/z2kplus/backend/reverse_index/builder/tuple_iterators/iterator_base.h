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
#include "kosak/coding/failures.h"

namespace z2kplus::backend::reverse_index::builder::tuple_iterators {

template<typename Tuple>
class TupleIterator {
  typedef kosak::coding::FailFrame FailFrame;
public:
  TupleIterator() = default;
  DISALLOW_COPY_AND_ASSIGN(TupleIterator);
  DISALLOW_MOVE_COPY_AND_ASSIGN(TupleIterator);
  virtual ~TupleIterator() = default;

  virtual bool tryGetNext(std::optional<Tuple> *result, const FailFrame &ff) = 0;
  virtual void reset() = 0;
};
}   // namespace z2kplus::backend::reverse_index::builder::tuple_iterators
