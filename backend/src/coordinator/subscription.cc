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

#include "z2kplus/backend/coordinator/subscription.h"

#include <atomic>
#include "z2kplus/backend/shared/magic_constants.h"

using kosak::coding::streamf;
using kosak::coding::Unit;

using z2kplus::backend::reverse_index::iterators::IteratorContext;
using z2kplus::backend::reverse_index::iterators::ZgramIterator;
using z2kplus::backend::reverse_index::iterators::zgramRel_t;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::shared::protocol::Estimate;
using z2kplus::backend::shared::protocol::Estimates;
namespace magicConstants = z2kplus::backend::shared::magicConstants;

namespace z2kplus::backend::coordinator {
namespace {
std::atomic<uint64_t> nextFreeSubscriptionId(0);
}  // namespace

PerSideStatus PerSideStatus::create(const ConsolidatedIndex &index, const ZgramIterator *query,
    ZgramId recordId, bool forward, size_t minItems) {
  IteratorContext ctx(index, forward);
  auto zgOff = index.lowerBound(recordId);
  auto zgRel = ctx.offToRel(zgOff);
  if (!forward) {
    ++zgRel;
  }
  auto iteratorState = query->createState(ctx);
  PerSideStatus result(forward, std::move(iteratorState));
  result.topUp(index, query, zgRel, minItems);
  return result;
}

PerSideStatus::PerSideStatus() = default;
PerSideStatus::PerSideStatus(bool forward, std::unique_ptr<ZgramIteratorState> iteratorState) :
    forward_(forward), iteratorState_(std::move(iteratorState)),
    residual_(std::make_unique<std::deque<zgramRel_t>>()), exhaustVersion_(-1) {}
PerSideStatus::PerSideStatus(PerSideStatus &&) noexcept = default;
PerSideStatus &PerSideStatus::operator=(PerSideStatus &&) noexcept = default;
PerSideStatus::~PerSideStatus() = default;

bool PerSideStatus::topUp(const ConsolidatedIndex &index, const ZgramIterator *query,
    zgramRel_t lowerBound, size_t minItems) {
  IteratorContext ctx(index, forward_);
  while (residual_->size() < minItems) {
    if (isExhausted(index)) {
      return false;
    }
    zgramRel_t items[magicConstants::iteratorChunkSize];
    auto numItems = query->getMore(ctx, iteratorState_.get(), lowerBound, items,
        magicConstants::iteratorChunkSize);
    if (numItems == 0) {
      setExhausted(index);
      return false;
    }
    for (size_t i = 0; i < numItems; ++i) {
      residual_->push_back(items[i]);
    }
  }
  return true;
}

bool PerSideStatus::isExhausted(const ConsolidatedIndex &index) const {
  auto raw = forward_ ? index.zgramEndOff().raw() : 0;
  return exhaustVersion_.raw() == raw;
}

void PerSideStatus::setExhausted(const ConsolidatedIndex &index) {
  auto raw = forward_ ? index.zgramEndOff().raw() : 0;
  exhaustVersion_ = exhaustVersion_t(raw);
}

std::ostream &operator<<(std::ostream &s, const PerSideStatus &o) {
  return streamf(s, "%o,[iter state],[res],%o)", o.forward_, o.exhaustVersion_);
}

bool Subscription::tryCreate(const ConsolidatedIndex &index, std::shared_ptr<Profile> profile,
    std::unique_ptr<ZgramIterator> &&query, const SearchOrigin &start, size_t pageSize,
    size_t queryMargin, std::shared_ptr<Subscription> *result,
    const FailFrame &/*ff*/) {
  struct visitor_t {
    explicit visitor_t(const ConsolidatedIndex &index) : index_(index) {}

    void operator()(const Unit &/*unit*/) {
      zgramId_ = index_.zgramEnd();
    }

    void operator()(uint64 timestamp) {
      auto off = index_.lowerBound(timestamp);
      if (off == index_.zgramEndOff()) {
        zgramId_ = index_.zgramEnd();
        return;
      }
      zgramId_ = index_.getZgramInfo(off).zgramId();
    }

    void operator()(const ZgramId &zgramId) {
      zgramId_ = zgramId;
    }

    const ConsolidatedIndex &index_;
    ZgramId zgramId_;
  };
  visitor_t v(index);
  std::visit(v, start.payload());
  auto displayed = std::make_pair(v.zgramId_, v.zgramId_);

  subscriptionId_t id(nextFreeSubscriptionId++);
  auto res = std::make_shared<Subscription>(Private(), id, std::move(profile), std::move(query),
      pageSize, queryMargin, std::move(displayed));
  res->resetIndex(index);
  *result = std::move(res);
  return true;
}

Subscription::Subscription(Private, subscriptionId_t id, std::shared_ptr<Profile> &&profile,
    std::unique_ptr<ZgramIterator> &&query, size_t pageSize, size_t queryMargin,
    std::pair<ZgramId, ZgramId> displayed) : id_(id), profile_(std::move(profile)), query_(std::move(query)),
    pageSize_(pageSize), queryMargin_(queryMargin), displayed_(std::move(displayed)) {
}
Subscription::~Subscription() = default;

void Subscription::resetIndex(const ConsolidatedIndex &newIndex) {
  frontStatus_ = PerSideStatus::create(newIndex, query_.get(), displayed_.first, false, queryMargin_);
  backStatus_ = PerSideStatus::create(newIndex, query_.get(), displayed_.second, true, queryMargin_);
}

std::pair<Estimates, bool> Subscription::updateEstimates() {
  const auto fSize = std::min(frontStatus_.residual_->size(), queryMargin_);
  const auto bSize = std::min(backStatus_.residual_->size(), queryMargin_);
  auto newEstimates = Estimates::create(fSize, bSize, fSize < queryMargin_, bSize < queryMargin_);
  auto changed = lastEstimates_ != newEstimates;
  lastEstimates_ = newEstimates;
  return std::make_pair(newEstimates, changed);
}

void Subscription::updateDisplayed(ZgramId zgramId) {
  displayed_.first = std::min(displayed_.first, zgramId);
  displayed_.second = std::max(displayed_.second, zgramId.next());
}

std::ostream &operator<<(std::ostream &s, const Subscription &o) {
  return streamf(s,
      "Subscription(id=%o, profile=%o, query=%o, pgSize=%o, queryMargin=%o, disp=%o, fs=%o, bs=%o)",
    o.id_, *o.profile_, *o.query_, o.pageSize_, o.queryMargin_, o.displayed_,
    o.frontStatus_, o.backStatus_);
}
}  // namespace z2kplus::backend::server
