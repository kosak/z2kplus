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

#include "z2kplus/backend/reverse_index/iterators/zgram/metadata/having_reaction.h"

#include <map>
#include <memory>
#include <utility>

#include "kosak/coding/map_utils.h"
#include "z2kplus/backend/reverse_index/iterators/iterator_common.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/util/misc.h"
#include "z2kplus/backend/util/relative.h"

namespace z2kplus::backend::reverse_index::iterators::zgram::metadata {

using kosak::coding::maputils::tryFind;
using kosak::coding::merger::Merger;
using kosak::coding::streamf;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::reverse_index::metadata::DynamicMetadata;
using z2kplus::backend::reverse_index::metadata::FrozenMetadata;

namespace {
struct MyState final : public ZgramIteratorState {
  explicit MyState(const HavingReaction *owner) : owner_(owner) {}
  ~MyState() final = default;

  size_t getMore(const IteratorContext &ctx, zgramRel_t *result, size_t capacity);

  const HavingReaction *owner_ = nullptr;
};
}  // namespace

std::unique_ptr<HavingReaction> HavingReaction::create(std::string &&reaction) {
  return std::make_unique<HavingReaction>(Private(), std::move(reaction));
}

HavingReaction::HavingReaction(Private, std::string &&reaction) : reaction_(std::move(reaction)) {}
HavingReaction::~HavingReaction() = default;

std::unique_ptr<ZgramIteratorState> HavingReaction::createState(const IteratorContext &ctx) const {
  return std::make_unique<MyState>(this);
}

size_t HavingReaction::getMore(const IteratorContext &ctx, ZgramIteratorState *state,
    zgramRel_t lowerBound, zgramRel_t *result,
    size_t capacity) const {
  auto *ms = dynamic_cast<MyState *>(state);
  if (!ms->updateNextStart(ctx, lowerBound, capacity)) {
    return 0;
  }
  return ms->getMore(ctx, result, capacity);
}

void HavingReaction::dump(std::ostream &s) const {
  streamf(s, "HavingReaction(%o)", reaction_);
}

namespace {
template<typename Iter>
struct advancer_t {
  advancer_t(bool forward, Iter begin, Iter end) : forward_(forward), begin_(begin), end_(end) {}

  Iter operator()(Iter ip) const {
    if (forward_) {
      return std::next(ip);
    }
    if (ip == begin_) {
      return end_;
    }
    return std::prev(ip);
  }

  bool forward_;
  Iter begin_;
  Iter end_;
};

size_t MyState::getMore(const IteratorContext &ctx, zgramRel_t *result, size_t capacity) {
  typedef FrozenMetadata::reactionCounts_t::mapped_type fInner_t;
  typedef DynamicMetadata::reactionCounts_t::mapped_type dInner_t;
  fInner_t fEmpty;
  dInner_t  dEmpty;
  const fInner_t *fInnerToUse;
  const dInner_t *dInnerToUse;
  const auto &ci = ctx.ci();
  const auto &fi = ci.frozenIndex();
  if (!fi.metadata().reactionCounts().tryFind(owner_->reaction(), &fInnerToUse, fi.makeLess())) {
    fInnerToUse = &fEmpty;
  }
  if (!tryFind(ci.dynamicIndex().metadata().reactionCounts(), owner_->reaction(), &dInnerToUse)) {
    dInnerToUse = &dEmpty;
  }

  auto nextZgramId = ci.getZgramInfo(ctx.relToOff(this->nextStart_)).zgramId();

  // The trick in this code is that we are going to try to do use the same code to iterate either
  // forward or backward, rather than two versions, one of which would use forward iterators and
  // the other of which would use reverse iterators.
  //
  // Since std::map has bidirectional iterators, this should be easy enough: you iterate backwards
  // starting at std::prev(end) and ending at a point one beyond begin. Conceptually you would
  // use a sentinel like std::prev(begin) but such a thing is invalid. What we do is define our
  // own version of prev, where std::prev(begin) is defined to be end.
  auto fRange = fInnerToUse->equal_range(nextZgramId);
  auto dRange = dInnerToUse->equal_range(nextZgramId);

  // If going forward, the next place to look is (fRange or dRange).begin
  // If going backward, the next place to look is prev((fRange or dRange).end)

  // The other clever trick is, if going backwards, we define prev(begin) == end.
  // This way our termination condition is simple -- we can compare to 'end' regardless of the
  // direction we are going.

  advancer_t<fInner_t::const_iterator> fAdvancer(ctx.forward(), fInnerToUse->begin(), fInnerToUse->end());
  advancer_t<dInner_t::const_iterator> dAdvancer(ctx.forward(), dInnerToUse->begin(), dInnerToUse->end());

  fInner_t::const_iterator fCurrent, fEnd;
  dInner_t::const_iterator dCurrent, dEnd;
  if (ctx.forward()) {
    // For f, the range we need to process is [fRange.first, fInnerToUse.end)
    // Likewise for d.
    fCurrent = fRange.first;
    dCurrent = dRange.first;

    fEnd = fInnerToUse->end();
    dEnd = dInnerToUse->end();
  } else {
    // Wrapping our heads around the concept of using the iterators to go backwards:
    // For f, the range we need to process is [prev(fRange.second), prev(fInnerToUse->begin)
    // Likewise for d.
    // But this is with the proviso that our "prev" code wraps around at
    // the very beginning of the map... in other words it defines prev(map.begin()) as map.end().
    fCurrent = fAdvancer(fRange.second);
    dCurrent = dAdvancer(dRange.second);

    fEnd = fAdvancer(fInnerToUse->begin());
    dEnd = dAdvancer(dInnerToUse->begin());
  }

  auto *outputCurrent = result;
  const auto *outputEnd = result + capacity;

  const bool fwd = ctx.forward();
  auto compare = [fwd](const ZgramId &lhs, const ZgramId &rhs) {
    return fwd ? lhs.compare(rhs) : rhs.compare(lhs);
  };

  auto maybeEmit = [&ctx, &outputCurrent](const ZgramId &zgramId, int32_t total) {
    zgramOff_t zgOff;
    if (total != 0 && ctx.ci().tryFind(zgramId, &zgOff)) {
      *outputCurrent++ = ctx.offToRel(zgOff);
    }
  };

  while (fCurrent != fEnd && dCurrent != dEnd && outputCurrent != outputEnd) {
    auto cmp = compare(fCurrent->first, dCurrent->first);
    if (cmp < 0) {
      maybeEmit(fCurrent->first, fCurrent->second);
    } else {
      // if cmp == 0 or > 0
      maybeEmit(dCurrent->first, dCurrent->second);
      dCurrent = dAdvancer(dCurrent);
    }
    if (cmp <= 0) {
      fCurrent = fAdvancer(fCurrent);
    }
  }

  while (fCurrent != fEnd && outputCurrent != outputEnd) {
    maybeEmit(fCurrent->first, fCurrent->second);
    fCurrent = fAdvancer(fCurrent);
  }

  while (dCurrent != dEnd && outputCurrent != outputEnd) {
    maybeEmit(dCurrent->first, dCurrent->second);
    dCurrent = dAdvancer(dCurrent);
  }

  auto numItems = outputCurrent - result;
  if (numItems == 0) {
    nextStart_ = ctx.getIndexZgBoundsRel().second;
  } else {
    nextStart_ = outputCurrent[-1].addRaw(1);
  }
  return numItems;
}
}  // namespace
}  // namespace z2kplus::backend::reverse_index::iterators::zgram::metadata
