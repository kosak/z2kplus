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

namespace z2kplus::backend::reverse_index::iterators::zgram {

// This iterator returns zgrams that match its criteria.
// For each bit set in the mask 'includePopulated_', the filter will match if the corresponding
// zgram field is nonempty.
// For each bit set in the mask 'includeUnpopulated_', the filter will match if the corresponding
// zgram field is empty.
//
// Our current corpus never actually has empty fields, so I'm not sure how often this careful
// accounting will matter. However, being precise helps preserve the semantics of certain
// optimizations, which would otherwise make me have to think too hard.
//
// For example, WordAdapter(Pattern("*",mask)) reduces to WordAdapter(AnyWord(mask)) which reduces
// to PopOrNot(mask, none).
//
// And, I think there's a mathematical argument which suggests that And(empty list) should be
// everything, i.e. EveryZgram(all,all).
class PopOrNot final : public ZgramIterator {
  struct Private {};
public:
  static std::unique_ptr<ZgramIterator> create(FieldMask includePopulated,
    FieldMask includeUnpopulated);
  PopOrNot(Private, FieldMask includePopulated, FieldMask includeUnpopulated);
  ~PopOrNot() final;

  std::unique_ptr<ZgramIteratorState> createState(const IteratorContext &ctx) const final;
  size_t getMore(const IteratorContext &ctx, ZgramIteratorState *state, zgramRel_t lowerBound,
      zgramRel_t *result, size_t capacity) const final;

  bool matchesEverything() const final {
    return includePopulated_ == FieldMask::all && includeUnpopulated_ == FieldMask::all;
  }

  bool matchesNothing() const final {
    return includePopulated_ == FieldMask::none && includeUnpopulated_ == FieldMask::none;
  }

private:
  void dump(std::ostream &s) const final;

  FieldMask includePopulated_ = FieldMask::none;
  FieldMask includeUnpopulated_ = FieldMask::none;
};

}  // namespace z2kplus::backend::reverse_index::iterators::zgram

