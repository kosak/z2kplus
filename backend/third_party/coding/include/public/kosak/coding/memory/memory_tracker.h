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

namespace kosak::coding::memory {

class MemoryTracker {
  typedef kosak::coding::FailFrame FailFrame;

public:
  MemoryTracker() = default;
  DISALLOW_COPY_AND_ASSIGN(MemoryTracker);
  DEFINE_MOVE_COPY_AND_ASSIGN(MemoryTracker);
  virtual ~MemoryTracker() = default;

  virtual bool tryReserve(size_t bytesNeeded, const FailFrame &ff) = 0;
  virtual void release(size_t bytes) = 0;
  virtual size_t estimateMemoryUsed() const = 0;
  virtual bool resourcesExceeded() = 0;
};

};  // namespace kosak::coding::memory
