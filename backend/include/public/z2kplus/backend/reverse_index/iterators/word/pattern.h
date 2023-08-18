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
#include "z2kplus/backend/util/automaton/automaton.h"
#include "z2kplus/backend/util/misc.h"

namespace z2kplus::backend::reverse_index::iterators::word {
class Pattern final : public WordIterator {
  typedef z2kplus::backend::util::automaton::FiniteAutomaton FiniteAutomaton;

  struct Private {};
public:
  static std::unique_ptr<WordIterator> create(FiniteAutomaton &&dfa, FieldMask fieldMask);
  Pattern(Private, FiniteAutomaton &&dfa, FieldMask fieldMask);
  ~Pattern() final;

  std::unique_ptr<WordIteratorState> createState(const IteratorContext &ctx) const final;

  size_t getMore(const IteratorContext &ctx, WordIteratorState *state, wordRel_t lowerBound,
      wordRel_t *result, size_t capacity) const final;

private:
  void dump(std::ostream &s) const final;

  FiniteAutomaton dfa_;
  FieldMask fieldMask_ = FieldMask::none;
};

}  // namespace z2kplus::backend::reverse_index::iterators
