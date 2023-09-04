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

#include "z2kplus/backend/reverse_index/iterators/zgram/or.h"

#include "kosak/coding/dumping.h"
#include "z2kplus/backend/reverse_index/iterators/zgram/popornot.h"
#include "z2kplus/backend/util/misc.h"

namespace z2kplus::backend::reverse_index::iterators {

using z2kplus::backend::reverse_index::iterators::zgram::PopOrNot;

using kosak::coding::dumpDeref;
using kosak::coding::streamf;

namespace {
struct MyState final : public ZgramIteratorState {
  explicit MyState(std::vector<ZgramStreamer> &&streamers);
  ~MyState() final;

  bool getNextResult(const IteratorContext &ctx, zgramRel_t *result);

  std::vector<ZgramStreamer> streamers_;
};
}  // namespace

// Optimizations:
// * Factor out "No Zgram" nodes
// * "Every Zgram" nodes dominate.
// * Fold in the children of "Or" nodes
// * Simplify for lists of size 0 or 1
std::unique_ptr<ZgramIterator> Or::create(std::vector<std::unique_ptr<ZgramIterator>> &&children) {
  std::vector<std::unique_ptr<ZgramIterator>> result;
  for (auto &child : children) {
    if (child->matchesNothing()) {
      continue;
    }
    if (child->matchesEverything()) {
      return std::move(child);
    }
    std::vector<std::unique_ptr<ZgramIterator>> orChildren;
    if (child->tryReleaseOrChildren(&orChildren)) {
      for (auto &oc : orChildren) {
        result.push_back(std::move(oc));
      }
      continue;
    }
    result.push_back(std::move(child));
  }
  if (result.empty()) {
    return PopOrNot::create(FieldMask::none, FieldMask::none);
  }
  if (result.size() == 1) {
    return std::move(result[0]);
  }
//  bool sensitive = result.end() != std::find_if(result.begin(), result.end(),
//    [](const unique_ptr<ZgramIterator> &uz) {
//      return uz->isSensitiveToMetadata();
//    });
  result.shrink_to_fit();
  return std::make_unique<Or>(Private(), std::move(result));
}

Or::Or(Private, std::vector<std::unique_ptr<ZgramIterator>> &&children) :
    children_(std::move(children)) {}
Or::~Or() = default;

std::unique_ptr<ZgramIteratorState> Or::createState(const IteratorContext &ctx) const {
  std::vector<ZgramStreamer> streamers;
  streamers.reserve(children_.size());
  for (const auto &child : children_) {
    streamers.emplace_back(child.get(), child->createState(ctx));
  }
  return std::make_unique<MyState>(std::move(streamers));
}

bool Or::tryReleaseOrChildren(std::vector<std::unique_ptr<ZgramIterator>> *result) {
  *result = std::move(children_);
  return true;
}

size_t Or::getMore(const IteratorContext &ctx, ZgramIteratorState *state, zgramRel_t lowerBound,
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

void Or::dump(std::ostream &s) const {
  streamf(s, "Or(%o)", dumpDeref(children_.begin(), children_.end(), "[", "]", ", "));
}

namespace {
MyState::MyState(std::vector<ZgramStreamer> &&streamers) : streamers_(std::move(streamers)) {}
MyState::~MyState() = default;

bool MyState::getNextResult(const IteratorContext &ctx, zgramRel_t *result) {
  passert(!streamers_.empty());

  // Find the minimum streamer. We fail if all of them are exhausted.
  std::optional<zgramRel_t> minValue;
  for (auto &s : streamers_) {
    zgramRel_t thisValue;
    if (!s.tryGetOrAdvance(ctx, nextStart_, &thisValue)) {
      continue;
    }
    if (!minValue.has_value() || thisValue < *minValue) {
      minValue = thisValue;
    }
  }
  if (!minValue.has_value()) {
    return false;
  }
  *result = *minValue;
  nextStart_ = minValue->addRaw(1);
  return true;
}
}  // namespace

}  // namespace z2kplus::backend::reverse_index::iterators
