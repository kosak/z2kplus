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

#include "kosak/coding/cancellation.h"

#include <thread>
#include "kosak/coding/misc.h"

using kosak::coding::misc::waitForLogic;
using kosak::coding::misc::WaitValueResult;

namespace kosak::coding {
Cancellation::Cancellation() = default;
Cancellation::~Cancellation() = default;

WaitValueResult Cancellation::sleepFor(std::chrono::milliseconds timeout) {
  auto poll = [this]() {
    return hasBeenCancelled_ ? WaitValueResult::Cancelled : WaitValueResult::Timeout;
  };
  return waitForLogic(&mutex_, &condVar_, poll, timeout);
}

WaitValueResult Cancellation::wait(const ready_t &ready) {
  auto poll = [this, &ready]() {
    return hasBeenCancelled_ ? WaitValueResult::Cancelled : ready();
  };
  return waitForLogic(&mutex_, &condVar_, poll, {});
}

void Cancellation::cancel() {
  mutex_.lock();
  bool oldValue = hasBeenCancelled_;
  hasBeenCancelled_ = true;
  mutex_.unlock();
  if (!oldValue) {
    condVar_.notify_all();
  }
}

bool Cancellation::hasBeenCancelled() {
  std::lock_guard lock(mutex_);
  return hasBeenCancelled_;
}
}  // namespace kosak::coding
