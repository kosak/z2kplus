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

#include <cstdlib>
#include <memory>
#include "kosak/coding/coding.h"

namespace kosak::coding::memory {
template<typename T>
class MaybeInlinedBuffer {
  static constexpr size_t fixedSize = 10;
public:
  static constexpr bool useStack(size_t size) {
    return size <= fixedSize;
  }

  static constexpr size_t calcStackSize(size_t size) {
    return useStack(size) ? size : 0;
  }

  explicit MaybeInlinedBuffer(T *stackBuffer, size_t size);
  DISALLOW_COPY_AND_ASSIGN(MaybeInlinedBuffer);
  DISALLOW_MOVE_COPY_AND_ASSIGN(MaybeInlinedBuffer);
  ~MaybeInlinedBuffer() = default;

  T *get() const {
    return bufferToUse_;
  }

private:
  std::unique_ptr<T[]> dynamicBuffer_;
  T *bufferToUse_ = nullptr;
};

template<typename T>
MaybeInlinedBuffer<T>::MaybeInlinedBuffer(T *stackBuffer, size_t size) {
  if (useStack(size)) {
    bufferToUse_ = stackBuffer;
    return;
  }
  dynamicBuffer_ = std::make_unique<T[]>(size);
  bufferToUse_ = dynamicBuffer_.get();
}
}  // namespace kosak::coding::memory
