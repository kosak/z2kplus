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

class And final : public ZgramIterator {
  struct Private {};
public:
  static std::unique_ptr<ZgramIterator> create(
    std::vector<std::unique_ptr<ZgramIterator>> &&children);
  And(Private, std::vector<std::unique_ptr<ZgramIterator>> &&children);

  ~And() final;

  std::unique_ptr<ZgramIteratorState> createState(const IteratorContext &ctx) const final;
  size_t getMore(const IteratorContext &ctx, ZgramIteratorState *state, zgramRel_t lowerBound,
      zgramRel_t *result, size_t capacity) const final;

  bool tryReleaseAndChildren(std::vector<std::unique_ptr<ZgramIterator>> *result) final;

private:
  void dump(std::ostream &s) const final;

  std::vector<std::unique_ptr<ZgramIterator>> children_;
};

}  // namespace z2kplus::backend::reverse_index::iterators
