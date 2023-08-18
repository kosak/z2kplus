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

#include "z2kplus/backend/reverse_index/iterators/zgram/zgramid.h"

#include <string>

#include "kosak/coding/coding.h"
#include "z2kplus/backend/reverse_index/iterators/iterator_common.h"

namespace z2kplus::backend::reverse_index::iterators {

using kosak::coding::streamf;
using z2kplus::backend::shared::ZgramId;

namespace {
struct MyState final : public ZgramIteratorState {
  explicit MyState(const std::optional<zgramRel_t> &idRel);
  ~MyState() final;

  size_t getMore(zgramRel_t *result);

  std::optional<zgramRel_t> idRel_;
};
}  // namespace

std::unique_ptr<ZgramIdIterator> ZgramIdIterator::create(RecordId zgramId) {
  return std::make_unique<ZgramIdIterator>(Private(), zgramId);
}

ZgramIdIterator::ZgramIdIterator(Private, RecordId zgramId) : zgramId_(zgramId) {}
ZgramIdIterator::~ZgramIdIterator() = default;

std::unique_ptr<ZgramIteratorState> ZgramIdIterator::createState(const IteratorContext &ctx) const {
  std::optional<zgramRel_t> zgramRel;
  zgramOff_t zgramOff;
  if (ctx.ci().tryFind(zgramId_, &zgramOff)) {
    zgramRel = ctx.offToRel(zgramOff);
  }
  return std::make_unique<MyState>(zgramRel);
}

size_t ZgramIdIterator::getMore(const IteratorContext &ctx, ZgramIteratorState *state,
    zgramRel_t lowerBound, zgramRel_t *result, size_t capacity) const {
  auto *ms = dynamic_cast<MyState *>(state);
  if (!ms->updateNextStart(ctx, lowerBound, capacity)) {
    return 0;
  }
  // Don't need to pass capacity because the most we need is 1, and capacity is known nonzero,
  // and we're only trying to get one.
  return ms->getMore(result);
}

size_t MyState::getMore(zgramRel_t *result) {
  if (!idRel_.has_value() || *idRel_ < nextStart_) {
    return 0;
  }
  *result = *idRel_;
  nextStart_ = idRel_->addRaw(1);
  return 1;
}

void ZgramIdIterator::dump(std::ostream &s) const {
  streamf(s, "ZgramIdIterator(%o)", zgramId_);
}

namespace {
MyState::MyState(const std::optional<zgramRel_t> &idRel) : idRel_(idRel) {}
MyState::~MyState() = default;
}  // namespace
}  // namespace z2kplus::backend::reverse_index::iterators
