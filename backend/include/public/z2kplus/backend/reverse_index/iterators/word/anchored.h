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

namespace z2kplus::backend::reverse_index::iterators {

class Anchored final : public WordIterator {
  struct Private {};
public:
  static std::unique_ptr<WordIterator> create(std::unique_ptr<WordIterator> &&child,
    bool anchoredLeft, bool anchoredRight);

  Anchored(Private, std::unique_ptr<WordIterator> &&child, bool anchoredLeft, bool anchoredRight);
  ~Anchored() final;

  bool tryGetAnchorChild(std::unique_ptr<WordIterator> *child, bool *anchoredLeft,
      bool *anchoredRight) final;

  std::unique_ptr<WordIteratorState> createState(const IteratorContext &ctx) const final;

  size_t getMore(const IteratorContext &ctx, WordIteratorState *state, wordRel_t lowerBound,
      wordRel_t *result, size_t capacity) const final;

private:
  size_t applyFilter(const IteratorContext &ctx, WordIteratorState *state, wordRel_t *result,
      size_t capacity) const;
  void dump(std::ostream &s) const override;

  std::unique_ptr<WordIterator> child_;
  bool anchoredLeft_;
  bool anchoredRight_;
};

}  // namespace z2kplus::backend::reverse_index::iterators
