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

#include "z2kplus/backend/reverse_index/iterators/zgram/popornot.h"

namespace z2kplus::backend::reverse_index::iterators::zgram {

using kosak::coding::streamf;

namespace {
struct MyState final : public ZgramIteratorState {
  explicit MyState(FieldMask includePopulated, FieldMask includeUnpopulated);
  ~MyState() final;

  bool getNextResult(const IteratorContext &ctx, zgramRel_t zgEnd, zgramRel_t *result);

  FieldMask includePopulated_ = FieldMask::none;
  FieldMask includeUnpopulated_ = FieldMask::none;
};

FieldMask characterize(const ZgramInfo &zg);

template<typename T>
inline T enumAnd(T lhs, T rhs) {
  static_assert(std::is_enum_v<T>);
  return (T)((size_t)lhs & (size_t)rhs);
}
template<typename T>
inline T enumOr(T lhs, T rhs) {
  static_assert(std::is_enum_v<T>);
  return (T)((size_t)lhs | (size_t)rhs);
}
template<typename T>
inline T enumXor(T lhs, T rhs) {
  static_assert(std::is_enum_v<T>);
  return (T)((size_t)lhs ^ (size_t)rhs);
}
}  // namespace

std::unique_ptr<ZgramIterator> PopOrNot::create(FieldMask includePopulated,
  FieldMask includeUnpopulated) {
  return std::make_unique<PopOrNot>(Private(), includePopulated, includeUnpopulated);
}

PopOrNot::PopOrNot(Private, FieldMask includePopulated, FieldMask includeUnpopulated) :
  includePopulated_(includePopulated), includeUnpopulated_(includeUnpopulated) {}
PopOrNot::~PopOrNot() = default;

std::unique_ptr<ZgramIteratorState> PopOrNot::createState(const IteratorContext &ctx) const {
  return std::make_unique<MyState>(includePopulated_, includeUnpopulated_);
}

size_t PopOrNot::getMore(const IteratorContext &ctx, ZgramIteratorState *state,
    zgramRel_t lowerBound, zgramRel_t *result,
    size_t capacity) const {
  if (matchesNothing()) {
    return 0;
  }
  auto *ms = dynamic_cast<MyState *>(state);
  if (!ms->updateNextStart(ctx, lowerBound, capacity)) {
    return 0;
  }
  auto zgEnd = ctx.getIndexZgBoundsRel().second;
  for (size_t i = 0; i < capacity; ++i) {
    if (!ms->getNextResult(ctx, zgEnd, &result[i])) {
      return i;
    }
  }
  return capacity;
}

namespace {
bool MyState::getNextResult(const IteratorContext &ctx, zgramRel_t zgEnd, zgramRel_t *result) {
  const auto &ci = ctx.ci();
  while (true) {
    if (nextStart_ == zgEnd) {
      return false;
    }
    auto current = nextStart_++;
    if (includePopulated_ == FieldMask::all && includeUnpopulated_ == FieldMask::all) {
      *result = current;
      return true;
    }
    const auto &zg = ci.getZgramInfo(ctx.relToOff(current));
    auto popMask = characterize(zg);
    auto unpopMask = enumXor(popMask, FieldMask::all);
    if (enumAnd(includePopulated_, popMask) != FieldMask::none ||
        enumAnd(includeUnpopulated_, unpopMask) != FieldMask::none) {
      *result = current;
      return true;
    }
  }
}

FieldMask characterize(const ZgramInfo &zg) {
  static_assert((size_t)FieldMask::all == 0b1111);

  auto result = FieldMask::none;
  if (zg.senderWordLength() != 0) {
    result = enumOr(result, FieldMask::sender);
  }
  if (zg.signatureWordLength() != 0) {
    result = enumOr(result, FieldMask::signature);
  }
  if (zg.instanceWordLength() != 0) {
    result = enumOr(result, FieldMask::instance);
  }
  if (zg.bodyWordLength() != 0) {
    result = enumOr(result, FieldMask::body);
  }
  return result;
}
}

void PopOrNot::dump(std::ostream &s) const {
  streamf(s, "PopOrNot(pop=%o, unpop=%o)", includePopulated_, includeUnpopulated_);
}

namespace {
MyState::MyState(FieldMask includePopulated, FieldMask includeUnpopulated) :
    includePopulated_(includePopulated), includeUnpopulated_(includeUnpopulated) {}
MyState::~MyState() = default;
}  // namespace
}  // namespace z2kplus::backend::reverse_index::iterators::zgram
