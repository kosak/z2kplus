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

#include "z2kplus/backend/reverse_index/iterators/zgram/not.h"

namespace z2kplus::backend::reverse_index::iterators {

using kosak::coding::streamf;

namespace {
struct MyState final : public ZgramIteratorState {
  explicit MyState(ZgramStreamer &&streamer);
  ~MyState() final;

  size_t getMore(const IteratorContext &ctx, zgramRel_t *result, size_t capacity);

  ZgramStreamer streamer_;
  std::optional<zgramRel_t> lastChildHit_;
};
}  // namespace

std::unique_ptr<ZgramIterator> Not::create(std::unique_ptr<ZgramIterator> &&child) {
  std::unique_ptr<ZgramIterator> negatedChild;
  if (child->tryNegate(&negatedChild)) {
    return negatedChild;
  }
  return std::make_unique<Not>(Private(), std::move(child));
}

Not::Not(Private, std::unique_ptr<ZgramIterator> &&child) : child_(std::move(child)) {}
Not::~Not() = default;

bool Not::tryNegate(std::unique_ptr<ZgramIterator> *result) {
  *result = std::move(child_);
  return true;
}

std::unique_ptr<ZgramIteratorState> Not::createState(const IteratorContext &ctx) const {
  const auto *child = child_.get();
  return std::make_unique<MyState>(ZgramStreamer(child, child->createState(ctx)));
}

size_t Not::getMore(const IteratorContext &ctx, ZgramIteratorState *state, zgramRel_t lowerBound,
    zgramRel_t *result, size_t capacity) const {
  auto *ms = dynamic_cast<MyState *>(state);
  if (!ms->updateNextStart(ctx, lowerBound, capacity)) {
    return 0;
  }
  return ms->getMore(ctx, result, capacity);
}

namespace {
MyState::MyState(ZgramStreamer &&streamer) : streamer_(std::move(streamer)) {}
MyState::~MyState() = default;

size_t MyState::getMore(const IteratorContext &ctx, zgramRel_t *result, size_t capacity) {
  size_t i = 0;
  auto zgEnd = ctx.getIndexZgBoundsRel().second;
  while (true) {
    if (!lastChildHit_.has_value()) {
      zgramRel_t temp;
      if (streamer_.tryGetOrAdvance(ctx, nextStart_, &temp)) {
        lastChildHit_ = temp;
      }
    }
    while (true) {
      if (i == capacity || nextStart_ == zgEnd) {
        return i;
      }
      if (lastChildHit_.has_value() && nextStart_ == *lastChildHit_) {
        ++nextStart_;
        lastChildHit_.reset();
        break;
      }
      result[i++] = nextStart_++;
    }
  }
}
}  // namespace

void Not::dump(std::ostream &s) const {
  streamf(s, "Not(%o)", *child_);
}

}  // namespace z2kplus::backend::reverse_index::iterators
