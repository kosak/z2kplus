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

#include "z2kplus/backend/reverse_index/iterators/boundary/near.h"

#include "kosak/coding/dumping.h"
#include "z2kplus/backend/reverse_index/iterators/boundary/word_adaptor.h"
#include "z2kplus/backend/reverse_index/iterators/zgram/popornot.h"
#include "z2kplus/backend/util/misc.h"

namespace z2kplus::backend::reverse_index::iterators::boundary {

using kosak::coding::streamf;
using kosak::coding::dumpDeref;
using z2kplus::backend::reverse_index::iterators::zgram::PopOrNot;

namespace {
class MyState final : public ZgramIteratorState {
public:
  MyState(size_t margin, std::unique_ptr<WordStreamer[]> &&streamers,
      size_t numStreamers);
  ~MyState() final;

  bool getNextResult(const IteratorContext &ctx, zgramRel_t *result, wordRel_t lowerBound);

  size_t getMore(const IteratorContext &ctx, zgramRel_t *result, size_t capacity);

private:
  size_t margin_ = 0;
  std::unique_ptr<WordStreamer[]> streamers_;
  size_t numStreamers_ = 0;
};
}  // namespace

// Optimizations:
// * The empty adjacency list matches every zgram
// * The adjacency list with one word in it can be more simply handled by WordAdaptor.
std::unique_ptr<ZgramIterator> Near::create(size_t margin,
    std::vector<std::unique_ptr<WordIterator>> &&children) {
  if (children.empty()) {
    return PopOrNot::create(FieldMask::all, FieldMask::all);
  }
  if (children.size() == 1) {
    return WordAdaptor::create(std::move(children[0]));
  }
  return std::make_unique<Near>(Private(), margin, std::move(children));
}

Near::Near(Private, size_t margin, std::vector<std::unique_ptr<WordIterator>> children) :
  margin_(margin), children_(std::move(children)) {}

Near::~Near() = default;

std::unique_ptr<ZgramIteratorState> Near::createState(const IteratorContext &ctx) const {
  auto numStreamers = children_.size();
  auto streamers = std::make_unique<WordStreamer[]>(numStreamers);
  for (size_t i = 0; i < numStreamers; ++i) {
    const auto &c = children_[i];
    auto childState = c->createState(ctx);
    streamers[i] = WordStreamer(c.get(), std::move(childState));
  }
  if (!ctx.forward()) {
    std::reverse(streamers.get(), streamers.get() + numStreamers);
  }
  return std::make_unique<MyState>(margin_, std::move(streamers), children_.size());
}

// The definition is as follows. We have N children (N >= 2) and we want to get them all to be
// "near" each other. "near" is defined as:
// 1. They are all in the same zgram, and in the same field within that zgram.
// 2. child[i] is before child[next(i)] with a distance <= margin
//
// Special note for reverse iteration. If the state indicates reverse iteration, then we also
// reverse the order of the children. So a forward query for "^hello there corey" would be adjusted
// to read "corey there ^hello" in the reverse direction. Notice that, as long as you keep your
// coordinate systems straight, you do not need to adjust the anchors. In both cases that ^hello
// means, match if hello is at the start of field (in the absolute coordinate system).
//
// The strategy is:
// 1. Establish lower bounds and monotonicity:
//    (a) ensure child[i] is at least the lowerBound, and advance if it not.
//    (b) ensure child[i + 1] is ahead of child[i] and advance it if not.
//    If any child exhausts during this process, we also exhaust.
// 2. We look at the "rightmost" child (remember we have already adjusted the children for
//    forward/reverse iteration) and get its zgram identifier and field identifier.
// 3. Set the lower bound to the start of this field and do a pass over the children, again
///   advancing them and monotonically increasing them. As usual, if you exhaust you are
//    done.
// 4. At this point, all the children may be still within the original zg/field you intended them
//    to be, or they may have advanced (lucky you) all to some new zg/field, or it's some random mix
//    of zg/fields. Look at all the children and ask yourself if they all have the identical zg/field
//    (not the original one you were looking for in step 3, but SOME zg/field that is the same for
//    all children). If not, this example isn't going to work, so go back to step 1.
// 5. Our only remaining issue is distancing. Working from right to left this time, make sure
//    that child[i] is within 'margin_' words of child[i+1]. If it is not, advance it to that point.
//    If you exhaust, then we also exhaust. If you overshoot past child[i+1] then this case isn't
//    going to work, so go back to step 1. If you do not overshoot, continue to the left. The reason
//    we work from right to left is some kind inductive proof: shrinking the distance from child[i]
//    to child[i+1] (without overshooting it) may break the invariants of children to the left, but
//    it will not break the invariants of children to the right.
// 6. You have a complete example, so add this zgram to the results and start again and the next
//    zgram.
size_t Near::getMore(const IteratorContext &ctx, ZgramIteratorState *state, zgramRel_t lowerBound,
    zgramRel_t *result, size_t capacity) const {
  auto *ms = dynamic_cast<MyState *>(state);
  if (!ms->updateNextStart(ctx, lowerBound, capacity)) {
    return 0;
  }
  return ms->getMore(ctx, result, capacity);
}

void Near::dump(std::ostream &s) const {
  streamf(s, "Near(%o, %o)", margin_, dumpDeref(children_.begin(), children_.end(), "[", "]", ", "));
}

MyState::MyState(size_t margin, std::unique_ptr<WordStreamer[]> &&streamers,
    size_t numStreamers) : margin_(margin),
    streamers_(std::move(streamers)), numStreamers_(numStreamers) {}
MyState::~MyState() = default;

size_t MyState::getMore(const IteratorContext &ctx, zgramRel_t *result, size_t capacity) {
  const auto &ci = ctx.ci();
  auto bounds = ctx.getIndexZgBoundsRel();
  for (size_t i = 0; i < capacity; ++i) {
    if (nextStart_ == bounds.second) {
      return i;
    }
    auto wordLowerBound = ctx.getWordBoundsRel(ci.getZgramInfo(ctx.relToOff(nextStart_))).first;
    zgramRel_t zgr;
    if (!getNextResult(ctx, &zgr, wordLowerBound)) {
      return i;
    }
    result[i] = zgr;
    nextStart_ = zgr.addRaw(1);
  }
  return capacity;
}

bool MyState::getNextResult(const IteratorContext &ctx, zgramRel_t *result,
    wordRel_t wordLowerBound) {
  passert(numStreamers_ >= 2);
  wordRel_t positions[numStreamers_];

  // Helper methods
  auto ensureMonotonic = [this, &ctx, &positions](wordRel_t lb) {
    for (size_t i = 0; i < numStreamers_; ++i) {
      if (!streamers_[i].tryGetOrAdvance(ctx, lb, &positions[i])) {
        return false;
      }
      lb = positions[i].addRaw(1);
    }
    return true;
  };

  auto allSameWordInfo = [this, &ctx, &positions] {
    const auto &leftWi = ctx.ci().getWordInfo(ctx.relToOff(positions[0]));
    for (size_t i = 1; i < numStreamers_; ++i) {
      const auto &nextWi = ctx.ci().getWordInfo(ctx.relToOff(positions[i]));
      if (leftWi != nextWi) {
        return false;
      }
    }
    return true;
  };

  enum class EnforceResult { Abort, Continue, Valid };

  auto enforceMaximumDistance = [this, &ctx, &positions] {
    // Move from right to left, looking at the distance between children. If a child is too far
    // away from its immediate neighbor to the right, try to advance it to a conformant distance.
    // If we overshoot, then the outer loop needs to continue. If we exhaust then we need to exit
    // the outer method.
    for (size_t ir = numStreamers_ - 1; ir != 0; --ir) {
      auto lIndex = ir - 1;
      auto rIndex = ir;
      auto &lhs = streamers_[lIndex];
      auto lpos = positions[lIndex];
      auto rpos = positions[rIndex];
      auto distance = rpos - lpos;
      if (distance.raw() <= margin_) {
        // This child is ok!
        continue;
      }
      // Too far away. Advance it to at least the proper point (this may overshoot or exhaust).
      auto target = rpos.subtractRaw(margin_);
      wordRel_t newRel;
      if (!lhs.tryGetOrAdvance(ctx, target, &newRel)) {
        // Exhausted, so abort the whole method.
        return EnforceResult::Abort;
      }

      if (newRel >= rpos) {
        // Overshot. Tell the method to keep trying.
        return EnforceResult::Continue;
      }
    }

    return EnforceResult::Valid;
  };

  const auto &ci = ctx.ci();
  while (true) {
    // 1. Establish monotonicity
    if (!ensureMonotonic(wordLowerBound)) {
      return false;
    }

    // 3. Establish monotonicity again, this time from the word position that is the start of
    // the field of the rightmost streamer
    //auto &rightStr = streamers_[numStreamers_ - 1];
    {
      // These soon become stale so we hide them inside a scope so you don't accidentally use them.
      const auto &rightWi = ci.getWordInfo(ctx.relToOff(positions[numStreamers_ - 1]));
      const auto &rightZg = ci.getZgramInfo(rightWi.zgramOff());
      auto rightFieldStart = ctx.getFieldBoundsRel(rightZg, rightWi.fieldTag()).first;
      if (!ensureMonotonic(rightFieldStart)) {
        return false;
      }
    }

    // 4. Confirm that all have the same wordInfo (meaning zgram id and tag)
    if (!allSameWordInfo()) {
      continue;
    }

    auto ef = enforceMaximumDistance();
    switch (ef) {
      case EnforceResult::Abort:
        return false;
      case EnforceResult::Continue:
        continue;
      case EnforceResult::Valid:
        const auto &wi = ci.getWordInfo(ctx.relToOff(positions[numStreamers_ - 1]));
        *result = ctx.offToRel(wi.zgramOff());
        return true;
    }
    notreached;
  }
}
}  // namespace z2kplus::backend::reverse_index::iterators::boundary

