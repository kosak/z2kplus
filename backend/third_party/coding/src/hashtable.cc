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

#include "kosak/coding/hashtable.h"

namespace kosak::coding  {
namespace internal {

HashtableBase::HashtableBase() = default;
HashtableBase::HashtableBase(double loadFactor, size_t resizeMultiplier, size_t resizeAdder,
  MemoryTracker *memoryTracker, size_t capacity, size_t size, size_t growThreshold) :
  loadFactor_(loadFactor), resizeMultiplier_(resizeMultiplier), resizeAdder_(resizeAdder),
  memoryTracker_(memoryTracker), capacity_(capacity), size_(size), growThreshold_(growThreshold) {
}
HashtableBase::HashtableBase(HashtableBase &&) noexcept = default;
HashtableBase &HashtableBase::operator=(HashtableBase &&) noexcept = default;

void HashtableBase::calcCapacityAndGrowThreshold(size_t initialNumBuckets, double loadFactor,
  size_t *capacity, size_t *growThreshold) {
  const auto *prime = std::lower_bound(internal::primes.begin(), internal::primes.end(),
    initialNumBuckets);
  passert(prime != internal::primes.end());

  *capacity = *prime;
  *growThreshold = (size_t)(*capacity * loadFactor);
}

void HashtableBase::assertCapacityNotZero() const {
  if (capacity_ == 0) {
    debug("WHY");
  }
  passert(capacity_ != 0);
}
}  // namespace internal
}  // namespace kosak::coding
