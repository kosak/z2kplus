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

#include <cctype>
#include "kosak/coding/coding.h"

namespace kosak::coding::memory {
namespace internal {
static constexpr size_t cacheLineSize = 64;
}  // namespace internal
template<typename T>
class alignas(internal::cacheLineSize) CacheLine {
  static_assert(sizeof(T) < internal::cacheLineSize);

public:
  template<typename ...ARGS>
  explicit CacheLine(ARGS &&... args) : item_(std::forward<ARGS>(args)...) {}

  ~CacheLine() = default;

  T *get() { return &item_; }
  const T *get() const { return &item_; }

  T &operator*() { return item_; }
  const T &operator*() const { return item_; }

  T *operator->() { return &item_; }
  const T *operator->() const { return &item_; }

private:
  T item_;
  char empty_[internal::cacheLineSize - sizeof(T)] = {0};
};
};  // namespace kosak::coding::memory

