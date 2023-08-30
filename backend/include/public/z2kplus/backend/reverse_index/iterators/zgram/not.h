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

#include "z2kplus/backend/reverse_index/iterators/iterator_common.h"

namespace z2kplus::backend::reverse_index::iterators {

class Not final : public ZgramIterator {
  struct Private {};
public:
  static std::unique_ptr<ZgramIterator> create(std::unique_ptr<ZgramIterator> &&child);
  Not(Private, std::unique_ptr<ZgramIterator> &&child);
  ~Not() final;

  std::unique_ptr<ZgramIteratorState> createState(const IteratorContext &ctx) const final;
  /**
   * Fills the 'result' buffer with up to 'capacity' zgramRel_t matching items, where the items so added
   * are beyond both the previously-returned items and >= 'lowerBound'.
   * @param ctx Global iterator context, like access to the index and whether the iterator is forward or backwards
   * @param state This iterator's local state
   * @param lowerBound Lower bound for returned results
   * @param result The result buffer
   * @param capacity The capacity of the result buffer
   * @return Number of items actually found
   */
  size_t getMore(const IteratorContext &ctx, ZgramIteratorState *state, zgramRel_t lowerBound,
      zgramRel_t *result, size_t capacity) const final;

  bool tryNegate(std::unique_ptr<ZgramIterator> *result) final;

private:
  void dump(std::ostream &s) const final;

  std::unique_ptr<ZgramIterator> child_;
};

}  // namespace z2kplus::backend::reverse_index::iterators
