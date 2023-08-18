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
#include "kosak/coding/memory/mapped_file.h"
#include "kosak/coding/text/misc.h"
#include "z2kplus/backend/reverse_index/builder/common.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/iterator_base.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/tuple_serializer.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/util.h"

namespace z2kplus::backend::reverse_index::builder::tuple_iterators {
template<typename Tuple>
class RowIterator final : public TupleIterator<Tuple> {
  typedef kosak::coding::FailFrame FailFrame;
  template<typename T>
  using MappedFile = kosak::coding::memory::MappedFile<T>;

public:
  explicit RowIterator(MappedFile<char> &&mf) : iter_(std::move(mf)) {}
  ~RowIterator() = default;

  bool tryGetNext(std::optional<Tuple> *result, const FailFrame &ff) final;
  void reset() final {
    iter_.reset();
  }

private:
  RecordIterator iter_;
};

template<typename Tuple>
bool RowIterator<Tuple>::tryGetNext(std::optional<Tuple> *result, const FailFrame &ff) {
  std::string_view record;
  if (!iter_.getNext(&record)) {
    result->reset();
    return true;
  }
  *result = Tuple();
  using z2kplus::backend::reverse_index::builder::tuple_iterators::tupleSerializer::tryParseTuple;
  auto res = tryParseTuple(record, defaultFieldSeparator, &**result, ff.nest(KOSAK_CODING_HERE));
  return res;
}
}   // namespace z2kplus::backend::reverse_index::builder::tuple_iterators
