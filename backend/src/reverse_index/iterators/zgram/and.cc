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

#include "z2kplus/backend/reverse_index/iterators/zgram/and.h"

#include <algorithm>
#include <numeric>
#include "kosak/coding/dumping.h"
#include "z2kplus/backend/reverse_index/iterators/zgram/popornot.h"
#include "z2kplus/backend/util/misc.h"

namespace z2kplus::backend::reverse_index::iterators {

using kosak::coding::dumpDeref;
using z2kplus::backend::reverse_index::iterators::zgram::PopOrNot;

namespace {
struct MyState final : public ZgramIteratorState {
  explicit MyState(std::vector<ZgramStreamer> &&streamers);
  ~MyState() final;

  bool getNextResult(const IteratorContext &ctx, zgramRel_t *result);

  std::vector<ZgramStreamer> streamers_;
};
}  // namespace

// Optimizations:
// * Factor out "Every Zgram" nodes
// * "No Zgram" nodes dominate.
// * Fold in the children of "And" nodes
// * Simplify for lists of size 0 or 1
std::unique_ptr<ZgramIterator> And::create(std::vector<std::unique_ptr<ZgramIterator>> &&children) {
  std::vector<std::unique_ptr<ZgramIterator>> result;
  for (auto &child : children) {
    if (child->matchesEverything()) {
      continue;
    }
    if (child->matchesNothing()) {
      return std::move(child);
    }
    std::vector<std::unique_ptr<ZgramIterator>> andChildren;
    if (child->tryReleaseAndChildren(&andChildren)) {
      for (auto &ac : andChildren) {
        result.push_back(std::move(ac));
      }
      continue;
    }
    result.push_back(std::move(child));
  }
  if (result.empty()) {
    return PopOrNot::create(FieldMask::all, FieldMask::all);
  }
  if (result.size() == 1) {
    return std::move(result[0]);
  }
  result.shrink_to_fit();
  return std::make_unique<And>(Private(), std::move(result));
}

And::And(Private, std::vector<std::unique_ptr<ZgramIterator>> &&children) :
  children_(std::move(children)) {}
And::~And() = default;

std::unique_ptr<ZgramIteratorState> And::createState(const IteratorContext &ctx) const {
  std::vector<ZgramStreamer> streamers;
  streamers.reserve(children_.size());
  for (const auto &child : children_) {
    streamers.emplace_back(child.get(), child->createState(ctx));
  }
  return std::make_unique<MyState>(std::move(streamers));
}

size_t And::getMore(const IteratorContext &ctx, ZgramIteratorState *state, zgramRel_t lowerBound,
    zgramRel_t *result, size_t capacity) const {
  auto *ms = dynamic_cast<MyState *>(state);
  if (!ms->updateNextStart(ctx, lowerBound, capacity)) {
    return 0;
  }
  for (size_t i = 0; i < capacity; ++i) {
    if (!ms->getNextResult(ctx, &result[i])) {
      return i;
    }
  }
  return capacity;
}

bool And::tryReleaseAndChildren(std::vector<std::unique_ptr<ZgramIterator>> *result) {
  *result = std::move(children_);
  return true;
}

void And::dump(std::ostream &s) const {
  streamf(s, "And(%o)", dumpDeref(children_.begin(), children_.end(), "[", "]", ", "));
}

namespace {
MyState::MyState(std::vector<ZgramStreamer> &&streamers) : streamers_(std::move(streamers)) {}
MyState::~MyState() = default;

bool MyState::getNextResult(const IteratorContext &ctx, zgramRel_t *result) {
  passert(!streamers_.empty());
  // We go round-robin over the streamers, wrapping around, until we've got 'numStreamers_' that
  // are exactly at 'nextStart_', or one of them exausts.
  size_t thisIndex = 0;
  size_t numInAgreement = 0;
  while (true) {
    auto &str = streamers_[thisIndex];
    zgramRel_t value;
    if (!str.tryGetOrAdvance(ctx, nextStart_, &value)) {
      return false;
    }
    if (value == nextStart_) {
      ++numInAgreement;
      if (numInAgreement == streamers_.size()) {
        *result = nextStart_;
        // next lower bound is one past here
        ++nextStart_;
        return true;
      }
    } else {
      nextStart_ = value;
      numInAgreement = 1;
    }
    ++thisIndex;
    if (thisIndex == streamers_.size()) {
      thisIndex = 0;
    }
  }
}
}  // namespace
}  // namespace z2kplus::backend::reverse_index::iterators
