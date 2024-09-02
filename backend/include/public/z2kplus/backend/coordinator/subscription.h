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

#include <deque>
#include <memory>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/strongint.h"
#include "z2kplus/backend/reverse_index/index/consolidated_index.h"
#include "z2kplus/backend/reverse_index/types.h"
#include "z2kplus/backend/reverse_index/iterators/iterator_common.h"
#include "z2kplus/backend/shared/protocol/misc.h"
#include "z2kplus/backend/shared/protocol/message/dresponse.h"

namespace z2kplus::backend::coordinator {
namespace internal {
struct ExhaustVersionTag {
  static constexpr const char text[] = "ExhaustVersion";
};
}  // namespace internal
typedef kosak::coding::StrongInt<size_t, internal::ExhaustVersionTag> exhaustVersion_t;


struct PerSideStatus {
  typedef z2kplus::backend::reverse_index::zgramOff_t zgramOff_t;
  typedef z2kplus::backend::reverse_index::index::ConsolidatedIndex ConsolidatedIndex;
  typedef z2kplus::backend::reverse_index::iterators::IteratorContext IteratorContext;
  typedef z2kplus::backend::reverse_index::iterators::ZgramIteratorState ZgramIteratorState;
  typedef z2kplus::backend::reverse_index::iterators::zgramRel_t zgramRel_t;
  typedef z2kplus::backend::reverse_index::iterators::ZgramIterator ZgramIterator;
  typedef z2kplus::backend::shared::ZgramId ZgramId;

  static PerSideStatus create(const ConsolidatedIndex &index, const ZgramIterator *query,
      ZgramId recordId, bool forward, size_t minItems);

  PerSideStatus();
  PerSideStatus(bool forward, std::unique_ptr<ZgramIteratorState> iteratorState);
  DISALLOW_COPY_AND_ASSIGN(PerSideStatus);
  DECLARE_MOVE_COPY_AND_ASSIGN(PerSideStatus);
  ~PerSideStatus();

  bool topUp(const ConsolidatedIndex &index, const ZgramIterator *query, zgramRel_t lowerBound,
      size_t minItems);

  bool isExhausted(const ConsolidatedIndex &index) const;
  void setExhausted(const ConsolidatedIndex &index);

  bool forward_ = false;
  std::unique_ptr<ZgramIteratorState> iteratorState_;
  // These are zgrams that I've looked up, beyond the limit of the user's search, in order to estimate how many
  // more zgrams there are beyond the user's search. It is a unique_ptr because I want to have a noexcept move
  // constructor.
  std::unique_ptr<std::deque<zgramRel_t>> residual_;
  // When doing a forward search, we might reach the end. This means our search is exhausted "for now", until the
  // corpus gets new zgrams. At that point the search might not be exhausted any more. If this value is equal to the
  // number of zgrams in the index, the search is exhausted. Otherwise, the search is not exhausted. One specific
  // constant we like to start with, to mean not exhausted, is size_t(-1).
  exhaustVersion_t exhaustVersion_;

  friend std::ostream &operator<<(std::ostream &s, const PerSideStatus &o);
};

namespace internal {
struct SubscriptionIdTag {
  static constexpr const char text[] = "SubscriptionId";
};
}  // namespace internal
typedef kosak::coding::StrongInt<uint64_t, internal::SubscriptionIdTag> subscriptionId_t;

class Subscription : public std::enable_shared_from_this<Subscription> {
  struct Private {};
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::reverse_index::index::ConsolidatedIndex ConsolidatedIndex;
  typedef z2kplus::backend::reverse_index::zgramOff_t zgramOff_t;
  typedef z2kplus::backend::reverse_index::iterators::ZgramIterator ZgramIterator;
  typedef z2kplus::backend::shared::protocol::Estimates Estimates;
  typedef z2kplus::backend::shared::protocol::message::DResponse DResponse;
  typedef z2kplus::backend::shared::Profile Profile;
  typedef z2kplus::backend::shared::SearchOrigin SearchOrigin;
  typedef z2kplus::backend::shared::ZgramId ZgramId;

public:
  static bool tryCreate(const ConsolidatedIndex &index, std::shared_ptr<Profile> profile,
      std::string humanReadableText, std::unique_ptr<ZgramIterator> &&query, const SearchOrigin &start, size_t pageSize,
      size_t queryMargin, std::shared_ptr<Subscription> *result, const FailFrame &ff);

  Subscription(Private, subscriptionId_t id, std::shared_ptr<Profile> &&profile,
      std::string humanReadableText, std::unique_ptr<ZgramIterator> &&query, size_t pageSize, size_t queryMargin,
      std::pair<ZgramId, ZgramId> displayed_);
  DISALLOW_COPY_AND_ASSIGN(Subscription);
  DISALLOW_MOVE_COPY_AND_ASSIGN(Subscription);
  ~Subscription();

  void resetIndex(const ConsolidatedIndex &newIndex);

  void updateDisplayed(ZgramId zgramId);

  std::pair<Estimates, bool> updateEstimates();

  subscriptionId_t id() const { return id_; }
  const std::shared_ptr<Profile> &profile() const { return profile_; }

  const std::string &humanReadableText() const { return humanReadableText_; }

  const ZgramIterator *query() const { return query_.get(); }

  size_t pageSize() const { return pageSize_; }
  size_t queryMargin() const { return queryMargin_; }

  PerSideStatus &frontStatus() { return frontStatus_; }
  const PerSideStatus &frontStatus() const { return frontStatus_; }

  PerSideStatus &backStatus() { return backStatus_; }
  const PerSideStatus &backStatus() const { return backStatus_; }

  std::pair<ZgramId, ZgramId> &displayed() { return displayed_; }
  const std::pair<ZgramId, ZgramId> &displayed() const { return displayed_; }

private:
  subscriptionId_t id_;
  // The profile of the logged-in user.
  std::shared_ptr<Profile> profile_;
  std::string humanReadableText_;
  std::unique_ptr<ZgramIterator> query_;
  size_t pageSize_ = 0;
  size_t queryMargin_ = 0;

  PerSideStatus frontStatus_;
  PerSideStatus backStatus_;

  Estimates lastEstimates_;

  // The range of Zgrams that our client is actually displaying.
  std::pair<ZgramId, ZgramId> displayed_;

  friend std::ostream &operator<<(std::ostream &s, const Subscription &o);
};
}  // namespace z2kplus::backend::coordinator
