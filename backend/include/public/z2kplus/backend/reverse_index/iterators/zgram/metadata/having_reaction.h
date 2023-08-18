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

#include <vector>
#include "kosak/coding/merger.h"
#include "z2kplus/backend/reverse_index/iterators/iterator_common.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/util/myiterator.h"

namespace z2kplus::backend::reverse_index::iterators::zgram::metadata {

class HavingReaction final : public ZgramIterator {
  struct Private {};
  typedef z2kplus::backend::shared::ZgramId RecordId;

public:
  static std::unique_ptr<HavingReaction> create(std::string &&reaction);
  HavingReaction(Private, std::string &&reaction);
  DISALLOW_COPY_AND_ASSIGN(HavingReaction);
  DISALLOW_MOVE_COPY_AND_ASSIGN(HavingReaction);
  ~HavingReaction() final;

  std::unique_ptr<ZgramIteratorState> createState(const IteratorContext &ctx) const final;

  size_t getMore(const IteratorContext &ctx, ZgramIteratorState *state, zgramRel_t lowerBound,
      zgramRel_t *result, size_t capacity) const final;

  const std::string &reaction() const { return reaction_; }

protected:
  void dump(std::ostream &s) const final;

private:
  std::string reaction_;
};
}  // namespace z2kplus::backend::reverse_index::iterators::zgram::metadata
