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

#if 0
#include "z2kplus/backend/util/myallocator.h"

#include <cstddef>
#include "kosak/coding/coding.h"

namespace z2kplus::backend::util {

void SimpleAllocator::allocateMulti(const size_t *aggregateSizes, const size_t *alignments,
  size_t numItems, void **results) {
  // Sanity check input and output.
  for (size_t i = 0; i < numItems; ++i) {
    passert(alignments[i] <= worstCaseAlignment_, i, alignments[i], worstCaseAlignment_);
  }
  allocateImpl(aggregateSizes, alignments, numItems, results);
  for (size_t i = 0; i < numItems; ++i) {
    auto start = results[i];
    auto alignment = alignments[i];
    if (start == nullptr) {
      continue;
    }
    passert((((uintptr_t)start) & (alignment - 1)) == 0, start, alignment);
  }
}

BufferPopulator::BufferPopulator(size_t worstCaseAlignment, void *buffer, size_t capacity) :
  SimpleAllocator(worstCaseAlignment), buffer_(buffer), offset_(0), capacity_(capacity) {}

void BufferPopulator::allocateImpl(const size_t *sizes, const size_t *alignments, size_t count,
  void **starts) {
  for (size_t i = 0; i < count; ++i) {
    // It will remain null if (a) sizes[i] == 0 or (b) we are measuring (buffer_ == nullptr)
    starts[i] = nullptr;

    auto size = sizes[i];
    auto alignment = alignments[i];
    if (size == 0) {
      // If size==0, don't even try to align.
      continue;
    }

    offset_ = (offset_ + alignment - 1) & ~(alignment - 1);
    if (buffer_ != nullptr) {
      passert(offset_ + size <= capacity_, offset_, size, capacity_);
      starts[i] = (void *)((uintptr_t)buffer_ + offset_);
    }
    offset_ += size;
  }
}

}  // namespace z2kplus::backend::util
#endif
