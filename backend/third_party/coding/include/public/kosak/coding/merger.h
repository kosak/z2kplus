// Copyright 2023 Corey Kosak
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

#include <queue>
#include <utility>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/priority_queue.h"

namespace kosak::coding::merger {

template<typename MyIterator, typename Less>
class Merger;

namespace internal {
template<typename MyIterator, typename Less>
class PQLess {
public:
  explicit PQLess(const Merger<MyIterator, Less> *outer) : outer_(outer) {}
  bool operator()(size_t lIndex, size_t rIndex) const;

private:
  const Merger<MyIterator, Less> *outer_;
};
}  // namespace internal

template<typename MyIterator, typename Less = std::less<typename MyIterator::item_type>>
class Merger {
public:
  typedef typename MyIterator::item_type item_type;
  typedef MyIterator myIterator_t;

  Merger();
  explicit Merger(std::vector<MyIterator> streams, Less less = Less());
  DISALLOW_COPY_AND_ASSIGN(Merger);
  DISALLOW_MOVE_COPY_AND_ASSIGN(Merger);
  ~Merger();

  std::vector<MyIterator> releaseStreams() { return std::move(streams_); }
  void resetStreams(std::vector<MyIterator> streams);
  void resetBoth(std::vector<MyIterator> streams, Less less);

  bool tryGetNext(std::vector<item_type> *items, std::vector<size_t> *whence);
  void skipTo(const item_type &item);

private:
  std::vector<MyIterator> streams_;
  std::vector<item_type> currents_;
  Less less_;
  PriorityQueue<size_t, internal::PQLess<MyIterator, Less>> pq_;

  friend class internal::PQLess<MyIterator, Less>;
};

template<typename MyIterator, typename Less>
Merger<MyIterator, Less>::Merger() : pq_(internal::PQLess<MyIterator, Less>(this)) {}

template<typename MyIterator, typename Less>
Merger<MyIterator, Less>::Merger(std::vector<MyIterator> streams, Less less)
  : less_(std::move(less)), pq_(internal::PQLess<MyIterator, Less>(this)) {
  resetStreams(std::move(streams));
}

template<typename MyIterator, typename Less>
Merger<MyIterator, Less>::~Merger() = default;

template<typename MyIterator, typename Less>
void Merger<MyIterator, Less>::resetStreams(std::vector<MyIterator> streams) {
  pq_.clear();
  streams_ = std::move(streams);
  currents_.resize(streams_.size());

  // Initialize the priority queue with the head from each non-empty stream.
  for (size_t i = 0; i < streams_.size(); ++i) {
    if (streams_[i].tryGetNext(&currents_[i])) {
      pq_.push(i);
    }
  }
}

template<typename MyIterator, typename Less>
void Merger<MyIterator, Less>::resetBoth(std::vector<MyIterator> streams, Less less) {
  pq_.clear();
  less_ = std::move(less);
  resetStreams(std::move(streams));
}

template<typename MyIterator, typename Less>
bool Merger<MyIterator, Less>::tryGetNext(std::vector<item_type> *items,
  std::vector<size_t> *whence) {
  if (pq_.empty()) {
    return false;
  }
  items->clear();
  whence->clear();
  while (true) {
    // Take the top stream out of the PQ and add its current value to the result.
    auto topIndex = pq_.top();
    pq_.pop();
    auto thisSlot = &currents_[topIndex];
    items->push_back(std::move(*thisSlot));
    whence->push_back(topIndex);

    // If the stream has a next value, push it back into the PQ.
    if (streams_[topIndex].tryGetNext(thisSlot)) {
      pq_.push(topIndex);
    }
    // If all the streams are exhausted, or the next item to pull from the stream does not belong
    // in the current group, then return. The return always succeeds because we always have at least
    // one item in our result.
    // Note 1: "Belong in the current group" means "equal to all items in the current group" (they
    // all compare equal to each other).
    // Note 2: we use result->back() as a proxy for all the items in the group. Any item in the
    // group would do.
    // Note 3: Since the PQ is ordered, the comparison we do to end the iteration and finish the
    // group is "current group is BEFORE (not EQUAL and not AFTER) next item to pull from the
    // stream. Now, it cannot possibly be AFTER, because the PQ is ordered, so this would never have
    // happened. So it's equal or before.We can tell if it's BEFORE or not by simply calling
    // 'less_'.
    if (pq_.empty() || less_(items->back(), currents_[pq_.top()])) {
      return true;
    }
  }
}

template<typename MyIterator, typename Less>
void Merger<MyIterator, Less>::skipTo(const item_type &item) {
  while (!pq_.empty()) {
    auto topIndex = pq_.top();
    auto thisStream = &streams_[topIndex];
    auto thisSlot = &currents_[topIndex];
    // If stream head is >= item, we are finished.
    // === !(stream head < item)
    if (!less_(*thisSlot, item)) {
      return;
    }
    pq_.pop();

    // Keep sucking data from this particular stream until either (a) it's exhausted or (b) the
    // stream head is >= item.
    // === !(stream head < item)
    while (thisStream->tryGetNext(thisSlot)) {
      if (!less_(*thisSlot, item)) {
        pq_.push(topIndex);
        break;  // Break out of the inner "while".
      }
    }
  }
}

namespace internal {
template<typename T, typename Less>
bool PQLess<T, Less>::operator()(size_t lIndex, size_t rIndex) const {
  const auto &lItem = outer_->currents_[lIndex];
  const auto &rItem = outer_->currents_[rIndex];
  if (outer_->less_(lItem, rItem)) {
    return true;
  }
  if (outer_->less_(rItem, lItem)) {
    return false;
  }
  return lIndex < rIndex;
}

}  // namespace internal

}  // namespace kosak::coding::merger
