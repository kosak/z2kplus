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

#include "z2kplus/backend/reverse_index/iterators/word/any_word.h"
#include "z2kplus/backend/util/misc.h"
#include "kosak/coding/coding.h"

namespace z2kplus::backend::reverse_index::iterators::word {

using kosak::coding::streamf;

namespace {
struct MyState : public WordIteratorState {
  size_t getMore(const IteratorContext &ctx, FieldMask fieldMask, wordRel_t *result,
      size_t capacity);
};
}  // namespace

std::unique_ptr<WordIterator> AnyWord::create(FieldMask fieldMask) {
  return std::make_unique<AnyWord>(Private(), fieldMask);
}

bool AnyWord::matchesAnyWord(FieldMask *fieldMask) const {
  *fieldMask = fieldMask_;
  return true;
}

std::unique_ptr<WordIteratorState> AnyWord::createState(const IteratorContext &/*ctx*/) const {
  return std::make_unique<MyState>();
}

size_t AnyWord::getMore(const IteratorContext &ctx, WordIteratorState *state, wordRel_t lowerBound,
    wordRel_t *result, size_t capacity) const {
  if (fieldMask_ == FieldMask::none) {
    return 0;
  }
  auto *ms = dynamic_cast<MyState*>(state);
  if (!ms->updateNextStart(ctx, lowerBound, capacity)) {
    return 0;
  }
  return ms->getMore(ctx, fieldMask_, result, capacity);
}

void AnyWord::dump(std::ostream &s) const {
  streamf(s, "AnyWord(%o)", fieldMask_);
}

namespace {
size_t MyState::getMore(const IteratorContext &ctx, FieldMask fieldMask, wordRel_t *result,
    size_t capacity) {
  auto wordEnd = ctx.getIndexWordBoundsRel().second;
  auto *dest = result;
  const auto *destEnd = result + capacity;
  const auto &ci = ctx.ci();
  while (nextStart_ != wordEnd) {
    auto current = nextStart_++;
    const auto &wi = ci.getWordInfo(ctx.relToOff(current));
    if (IteratorUtils::MaskContains(fieldMask, wi.fieldTag())) {
      *dest++ = current;
      if (dest == destEnd) {
        break;
      }
    }
  }
  return dest - result;
}
}  // namespace
}  // namespace z2kplus::backend::reverse_index::iterators::word

