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
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/memory/memory_tracker.h"

namespace kosak::coding::memory {

namespace internal {
struct Chunk;
}  // namespace internal

class Arena {
  typedef kosak::coding::FailFrame FailFrame;

public:
  Arena();
  Arena(size_t chunkSize, MemoryTracker *optionalMemoryTracker);
  DISALLOW_COPY_AND_ASSIGN(Arena);
  DECLARE_MOVE_COPY_AND_ASSIGN(Arena);
  ~Arena();

  // Always indicates success or failure in the return value. In the case of failure, if
  // optionalFailures is not null, will also record the reason for the failure there. Furtheremore,
  // in practice it can only fail if there is a memoryTracker. Finally, note that we give you an
  // array of uninitialized memory. In particular, we don't call constructors or destructors.
  // Initializing and cleaning up the objects is up to you.
  template<class T>
  bool tryAllocateTyped(size_t count, T **result, const FailFrame &ff) {
    void *voidResult;
    auto success = tryAllocate(count * sizeof(T), alignof(T), &voidResult, ff.nest(KOSAK_CODING_HERE));
    if (!success) {
      return false;
    }
    *result = (T*)(voidResult);
    return true;
  }

  bool tryAllocate(size_t bytes, size_t alignment, void **result, const FailFrame &ff);

private:
  bool tryAllocateHelper(size_t bytes, size_t alignment, void **result, const FailFrame &ff);

  size_t chunkSize_ = 0;
  // Optional memory tracker. Does not own.
  UnownedPtr<MemoryTracker> memoryTracker_;

  // Linked list of chunks (which need to be given back to the allocator). I own it (despite it
  // being of typed UnownedPtr) but I do the deallocation myself.
  UnownedPtr<internal::Chunk> head_;
  uintptr_t chunkCurrentByte_ = 0;
  uintptr_t chunkEndByte_ = 0;
};
};  // namespace kosak::coding::memory
