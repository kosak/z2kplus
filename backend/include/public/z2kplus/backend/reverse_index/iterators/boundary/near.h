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

#include <queue>
#include <vector>
#include "z2kplus/backend/reverse_index/iterators/iterator_common.h"

namespace z2kplus::backend::reverse_index::iterators::boundary {

// Near is another operator that straddles the boundary between words and zgrams. It returns
// a zgram index when all of its children are near each other (in word space, within a margin of
// 'margin_') and inside the same zgram and field. (They cannot span zgrams or fields).
class Near final : public ZgramIterator {
  static constexpr size_t childBufferSize = 128;

  struct Private {};

public:
  static std::unique_ptr<ZgramIterator> create(size_t margin,
    std::vector<std::unique_ptr<WordIterator>> &&children);
  Near(Private, size_t margin, std::vector<std::unique_ptr<WordIterator>> children);
  ~Near() final;

  std::unique_ptr<ZgramIteratorState> createState(const IteratorContext &ctx) const final;
  size_t getMore(const IteratorContext &ctx, ZgramIteratorState *state, zgramRel_t lowerBound,
      zgramRel_t *result, size_t capacity) const final;

private:
  void dump(std::ostream &s) const override;

  size_t margin_ = 0;
  std::vector<std::unique_ptr<WordIterator>> children_;
};
}  // namespace z2kplus::backend::reverse_index::iterators::boundary
