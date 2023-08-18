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

#include <algorithm>
#include <array>
#include <cstdlib>
#include <memory>
#include <utility>
#include "kosak/coding/coding.h"
#include "kosak/coding/memory/memory_tracker.h"

namespace kosak::coding {

namespace internal {
// Each prime in this list is about 10% higher than the previous prime.
static constexpr std::array<size_t, 201> primes = {
  3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 47, 53, 59, 67, 73, 83, 97,
  107, 127, 139, 157, 173, 191, 211, 233, 257, 283, 311, 347, 383, 421, 463,
  509, 563, 619, 683, 751, 827, 911, 1009, 1109, 1223, 1361, 1499, 1657,
  1823, 2011, 2213, 2437, 2683, 2953, 3251, 3581, 3943, 4337, 4783, 5261,
  5791, 6373, 7013, 7717, 8501, 9371, 10313, 11351, 12487, 13751, 15131,
  16649, 18313, 20147, 22171, 24391, 26833, 29527, 32479, 35729, 39301,
  43237, 47563, 52321, 57557, 63313, 69653, 76631, 84299, 92737, 102013,
  112223, 123449, 135799, 149381, 164321, 180773, 198851, 218737, 240623,
  264697, 291167, 320291, 352327, 387577, 426353, 469009, 515917, 567527,
  624311, 686761, 755437, 830981, 914117, 1005541, 1106099, 1216711,
  1338391, 1472239, 1619473, 1781449, 1959593, 2155579, 2371141, 2608283,
  2869117, 3156031, 3471647, 3818831, 4200731, 4620809, 5082907, 5591197,
  6150317, 6765361, 7441897, 8186089, 9004711, 9905183, 10895743, 11985343,
  13183883, 14502277, 15952543, 17547799, 19302593, 21232867, 23356159,
  25691779, 28261003, 31087117, 34195829, 37615423, 41377009, 45514739,
  50066221, 55072847, 60580141, 66638171, 73302007, 80632229, 88695457,
  97565009, 107321521, 118053697, 129859087, 142845029, 157129549,
  172842529, 190126823, 209139509, 230053459, 253058809, 278364689,
  306201173, 336821299, 370503433, 407553787, 448309181, 493140103,
  542454113, 596699531, 656369491, 722006443, 794207131, 873627857,
  960990689, 1057089773, 1162798757, 1279078639, 1406986517, 1547685187,
  1702453717, 1872699097, 2059969031, 2265965981, 2492562607, 2741818909,
  3016000807, 3317600903, 3649360993, 4014297101,
};

template<typename Entry>
class Slot;

template<typename Entry, typename Utils>
class HashtableIterator;

class HashtableBase {
protected:
  typedef kosak::coding::FailFrame FailFrame;
  typedef kosak::coding::memory::MemoryTracker MemoryTracker;

  static void calcCapacityAndGrowThreshold(size_t initialNumBuckets, double loadFactor,
    size_t *capacity, size_t *growThreshold);

  HashtableBase();
  HashtableBase(double loadFactor, size_t resizeMultiplier, size_t resizeAdder,
    MemoryTracker *memoryTracker, size_t capacity, size_t size, size_t growThreshold);
  DISALLOW_COPY_AND_ASSIGN(HashtableBase);
  DECLARE_MOVE_COPY_AND_ASSIGN(HashtableBase);
  ~HashtableBase() = default;

  void assertCapacityNotZero() const;

  double loadFactor_ = 0;
  size_t resizeMultiplier_ = 0;
  size_t resizeAdder_ = 0;
  // Does not own. We use a unique_ptr so the compiler-generated move does the right thing.
  std::unique_ptr<MemoryTracker, NoDelete> memoryTracker_;
  size_t capacity_ = 0;
  size_t size_ = 0;
  size_t growThreshold_ = 0;
};
}  // namespace internal

template<typename Entry, typename Utils>
class Hashtable : public internal::HashtableBase {
  typedef kosak::coding::FailFrame FailFrame;
public:
  typedef internal::Slot<Entry> slot_t;
  typedef internal::HashtableIterator<Entry, Utils> iterator_t;

  static bool tryCreate(size_t initialNumBuckets, double loadFactor, size_t resizeMultiplier,
    size_t resizeAdder, MemoryTracker *memoryTracker, Hashtable *result, const FailFrame &ff);

  Hashtable();
  DISALLOW_COPY_AND_ASSIGN(Hashtable);
  DECLARE_MOVE_COPY_AND_ASSIGN(Hashtable);
  ~Hashtable();

  // Can find any kind of key, if Utils has an Equal and Hashcode for it
  template<typename Key>
  slot_t findSlot(const Key &key);

  bool tryFinishInsert(internal::Slot<Entry> *slot, const FailFrame &ff);

  iterator_t iterator() { return iterator_t(this, -1, false); }

  size_t size() const { return size_; }
  size_t capacity() const { return capacity_; }

private:
  Hashtable(double loadFactor, size_t resizeMultiplier, size_t resizeAdder,
    MemoryTracker *memoryTracker, size_t capacity, size_t size, size_t growThreshold,
    std::unique_ptr<Entry[]> &&entries);

  bool tryRehash(const FailFrame &ff);

  template<typename Key>
  std::pair<size_t, size_t> calculateIndexAndDelta(const Key &key);

  std::unique_ptr<Entry[]> entries_;

  friend class internal::Slot<Entry>;
  friend class internal::HashtableIterator<Entry, Utils>;
};

namespace internal {
template<typename Entry>
class Slot {
public:
  Slot() = default;
  Slot(Entry *entry, bool found) : entry_(entry), found_(found) {}
  ~Slot() = default;

  Entry *entry() const { return entry_; }
  bool found() const { return found_; }

private:
  Entry *entry_ = nullptr;
  bool found_ = false;
};
}  // namespace internal

template<typename Entry, typename Utils>
bool Hashtable<Entry, Utils>::tryCreate(size_t initialNumBuckets, double loadFactor,
  size_t resizeMultiplier, size_t resizeAdder, MemoryTracker *memoryTracker, Hashtable *result,
    const FailFrame &ff) {
  size_t capacity, growThreshold;
  calcCapacityAndGrowThreshold(initialNumBuckets, loadFactor, &capacity, &growThreshold);

  size_t bytesNeeded = capacity * sizeof(Entry);
  if (memoryTracker != nullptr && !memoryTracker->tryReserve(bytesNeeded, ff.nest(KOSAK_CODING_HERE))) {
    return false;
  }
  std::unique_ptr<Entry[]> entries = std::make_unique<Entry[]>(capacity);
  for (size_t i = 0; i < capacity; ++i) {
    Utils::makeInvalid(&entries[i]);
  }
  *result = Hashtable(loadFactor, resizeMultiplier, resizeAdder, memoryTracker, capacity, 0,
      growThreshold, std::move(entries));
  return true;
}

template<typename Entry, typename Utils>
Hashtable<Entry, Utils>::Hashtable() = default;

template<typename Entry, typename Utils>
Hashtable<Entry, Utils>::Hashtable(double loadFactor, size_t resizeMultiplier, size_t resizeAdder,
  MemoryTracker *memoryTracker, size_t capacity, size_t size, size_t growThreshold,
  std::unique_ptr<Entry[]> &&entries) : HashtableBase(loadFactor, resizeMultiplier, resizeAdder,
    memoryTracker, capacity, size, growThreshold), entries_(std::move(entries)) {}

template<typename Entry, typename Utils>
Hashtable<Entry, Utils>::Hashtable(Hashtable &&other) noexcept = default;

template<typename Entry, typename Utils>
Hashtable<Entry, Utils> &Hashtable<Entry, Utils>::operator=(Hashtable &&other) noexcept {
  // Because I want to call my destructor so I give memory back to the memory tracker
  reconstructInPlace(this, std::move(other));
  return *this;
}

template<typename Entry, typename Utils>
Hashtable<Entry, Utils>::~Hashtable() {
  if (memoryTracker_ != nullptr) {
    memoryTracker_->release(capacity_ * sizeof(Entry));
  }
}

template<typename Entry, typename Utils>
template<typename Key>
internal::Slot<Entry> Hashtable<Entry, Utils>::findSlot(const Key &key) {
  assertCapacityNotZero();
  auto id = calculateIndexAndDelta(key);
  auto index = id.first;
  auto delta = id.second;

  while (true) {
    Entry *entry = &entries_[index];
    if (!Utils::isValid(*entry)) {
      return internal::Slot<Entry>(entry, false);
    }
    if (Utils::equal(*entry, key)) {
      return internal::Slot<Entry>(entry, true);
    }
    index = (index + delta) % capacity();
  }
}

template<typename Entry, typename Utils>
bool Hashtable<Entry, Utils>::tryFinishInsert(internal::Slot<Entry> *slot,
    const FailFrame &ff) {
  if (slot->found()) {
    return true;
  }
  ++size_;
  return size_ < growThreshold_ || tryRehash(ff.nest(KOSAK_CODING_HERE));
}

template<typename Entry, typename Utils>
bool Hashtable<Entry, Utils>::tryRehash(const FailFrame &ff) {
  auto newCapacity = capacity() * resizeMultiplier_ + resizeAdder_;
  Hashtable temp;
  if (!Hashtable::tryCreate(newCapacity, loadFactor_, resizeMultiplier_, resizeAdder_,
    memoryTracker_.get(), &temp, ff.nest(KOSAK_CODING_HERE))) {
    return false;
  }
  for (auto it = iterator(); it.moveNext(); ) {
    auto slot = temp.findSlot(*it.entry());
    *slot.entry() = std::move(*it.entry());
    if (!temp.tryFinishInsert(&slot, ff.nest(KOSAK_CODING_HERE))) {
      return false;
    }
  }
  *this = std::move(temp);
  return true;
}

template<typename Entry, typename Utils>
template<typename Key>
inline std::pair<size_t, size_t> Hashtable<Entry, Utils>::calculateIndexAndDelta(const Key &key) {
  auto id = Utils::hash(key);
  id.first %= capacity();  // index
  id.second %= capacity();  // delta
  if (id.second == 0) {  // delta must not be zero
    id.second = 1;
  }
  return id;
}

namespace internal {
template<typename Entry, typename Utils>
class HashtableIterator {
public:
  HashtableIterator(Hashtable<Entry, Utils> *owner, size_t index, bool exhausted) : owner_(owner),
    index_(index), exhausted_(exhausted) {}

  bool moveNext();

  Entry *entry() const { return &owner_->entries_[index_]; };

private:
  // Does not own.
  Hashtable<Entry, Utils> *owner_ = nullptr;
  size_t index_ = 0;
  bool exhausted_ = false;
};

template<typename Entry, typename Utils>
bool HashtableIterator<Entry, Utils>::moveNext() {
  if (exhausted_) {
    return false;
  }
  while (true) {
    ++index_;
    if (index_ >= owner_->capacity()) {
      exhausted_ = true;
      return false;
    }
    if (Utils::isValid(owner_->entries_[index_])) {
      return true;
    }
  }
}

}  // namespace internal
}  // namespace kosak::coding
