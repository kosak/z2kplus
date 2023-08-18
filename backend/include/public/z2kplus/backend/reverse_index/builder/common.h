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

#include <ostream>
#include "kosak/coding/coding.h"
#include "kosak/coding/delegate.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/memory/mapped_file.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/files/keys.h"
#include "z2kplus/backend/util/frozen/frozen_string_pool.h"

namespace z2kplus::backend::reverse_index::builder {
constexpr const char defaultRecordSeparator = 0;
constexpr const char defaultFieldSeparator = (char) 255;
constexpr const char wordOffSeparator = ';';

// Used with mmap. Is going to be a sparse file allocated on demand, so it doesn't matter if this
// number is kind of big.
 constexpr const size_t outputFileMaxSize = 100'000'000'000UL;

class RecordIterator {
  using FailFrame = kosak::coding::FailFrame;
  template<typename T>
  using MappedFile = kosak::coding::memory::MappedFile<T>;

public:
  explicit RecordIterator(MappedFile<char> &&mf);
  ~RecordIterator();

  bool getNext(std::string_view *result);

  void reset();

private:
  MappedFile<char> mf_;
  std::string_view remaining_;
};

class SimpleAllocator {
  using FailFrame = kosak::coding::FailFrame;
public:
  SimpleAllocator(char *start, size_t capacity, size_t initialAlignment);
  DISALLOW_COPY_AND_ASSIGN(SimpleAllocator);
  DISALLOW_MOVE_COPY_AND_ASSIGN(SimpleAllocator);
  ~SimpleAllocator() = default;

  template<typename T>
  bool tryAllocate(size_t numElements, T **result, const FailFrame &ff) {
    char *temp;
    if (!tryAllocate(numElements * sizeof(T), alignof(T), &temp, ff.nest(KOSAK_CODING_HERE))) {
      return false;
    }
    *result = (T*)temp;
    return true;
  }

  bool tryAllocate(size_t size, size_t alignment, char **result, const FailFrame &ff);

  bool tryAlign(size_t alignment, const FailFrame &ff);

  bool tryRealign(const FailFrame &ff) {
    return tryAlign(initialAlignment_, ff.nest(KOSAK_CODING_HERE));
  }

  size_t allocatedSize() const {
    return offset_;
  }

private:
  bool tryAdvance(size_t size, const FailFrame &ff);
  char *data() const { return start_ + offset_; }

  char *start_ = nullptr;
  size_t capacity_ = 0;
  size_t initialAlignment_ = 0;
  size_t offset_ = 0;
};
}  // namespace z2kplus::backend::reverse_index::builder
