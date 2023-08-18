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

#include "kosak/coding/memory/arena.h"

#include "kosak/coding/coding.h"
#include "kosak/coding/memory/memory_tracker.h"

namespace kosak::coding::memory {

using kosak::coding::FailFrame;
#define HERE KOSAK_CODING_HERE

namespace internal {
struct Chunk {
  Chunk() = default;
  Chunk(Chunk *next, size_t allocatedSize) : next_(next), allocatedSize_(allocatedSize) {}
  DISALLOW_COPY_AND_ASSIGN(Chunk);
  DISALLOW_MOVE_COPY_AND_ASSIGN(Chunk);
  ~Chunk() = default;

  Chunk *next_ = nullptr;
  size_t allocatedSize_ = 0;
  char data_[0];
};
}  // namespace internal

Arena::Arena() = default;
Arena::Arena(size_t chunkSize, MemoryTracker *optionalMemoryTracker) :
  chunkSize_(chunkSize), memoryTracker_(optionalMemoryTracker), head_(nullptr),
  chunkCurrentByte_(0), chunkEndByte_(0) {}
Arena::Arena(Arena &&) noexcept = default;
Arena &Arena::operator=(Arena &&) noexcept = default;
Arena::~Arena() {
  auto *chunk = head_.get();
  while (chunk != nullptr) {
    if (memoryTracker_ != nullptr) {
      memoryTracker_->release(chunk->allocatedSize_);
    }
    auto *next = chunk->next_;
    delete[] (char *)chunk;
    chunk = next;
  }
}

bool Arena::tryAllocate(size_t bytes, size_t alignment, void **result, const FailFrame &ff) {
  size_t alignmentMask = alignment - 1;
  // Alignment must be a power of 2.
  passert((alignment & alignmentMask) == 0);

  // If 'bytes' is larger than 10% of a chunk, then allocate it as an independent chunk.
  if (bytes > chunkSize_ / 10) {
    return tryAllocateHelper(bytes, alignment, result, ff.nest(HERE));
  }

  for (size_t i = 0; i < 2; ++i) {
    // Try to satisfy the memory request from the current chunk
    uintptr_t adjustedStart = (chunkCurrentByte_ + alignmentMask) & ~alignmentMask;
    if (adjustedStart <= chunkEndByte_) {
      auto end = adjustedStart + bytes;
      if (end <= chunkEndByte_) {
        chunkCurrentByte_ = end;
        *result = (void *)(adjustedStart);
        return true;
      }
    }

    // Close out the current chunk and start a new one
    void *chunk;
    if (!tryAllocateHelper(chunkSize_, 1, &chunk, ff.nest(HERE))) {
      return false;
    }
    chunkCurrentByte_ = (uintptr_t)chunk;
    chunkEndByte_ = chunkCurrentByte_ + chunkSize_;
  }
  crash("Should never get here");
}

bool Arena::tryAllocateHelper(size_t bytes, size_t alignment, void **result,
  const FailFrame &ff) {

  // The result of the call to 'new' should be aligned to at least alignof(void*).
  size_t alignmentToUse = alignment > alignof(void*) ? alignment : 1;
  size_t alignmentToUseMask = alignmentToUse - 1;

  size_t bytesNeeded = sizeof(internal::Chunk) + bytes + alignmentToUseMask;
  if (memoryTracker_ != nullptr && !memoryTracker_->tryReserve(bytesNeeded, ff.nest(HERE))) {
    return false;
  }

  char *chunkSpace = new char[bytesNeeded];
  head_.reset(new ((void*)chunkSpace) internal::Chunk(head_.get(), bytesNeeded));

  auto start = (uintptr_t)head_->data_;
  auto adjustedStart = (start + alignmentToUseMask) & ~alignmentToUseMask;
  *result = (void*)adjustedStart;
  return true;
}

}  // namespace kosak::coding::memory
