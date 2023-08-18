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

#include <condition_variable>
#include <functional>
#include <future>
#include <iterator>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include "kosak/coding/failures.h"

namespace kosak::coding::misc {
enum class WaitValueResult { Cancelled, Ready, Timeout };

/**
 * @param timeout Empty optional means wait forever. Zero means check and return immediately
 * (don't wait).
 */
WaitValueResult waitForLogic(std::mutex *mutex, std::condition_variable *condVar,
    const std::function<WaitValueResult()> &waitPoll,
    std::optional<std::chrono::milliseconds> timeout);

// A resetting optional is like an optional but which resets the optional at move time.
template<typename T>
class ResettingOptional {
public:
  ResettingOptional() = default;
  ResettingOptional(const T &value) : value_(value) {}
  explicit ResettingOptional(T &&value) : value_(std::forward<T>(value)) {}
  DISALLOW_COPY_AND_ASSIGN(ResettingOptional);
  ResettingOptional(ResettingOptional &&other) noexcept : value_(std::move(other.value_)) {
    other.value_.reset();
  }
  ResettingOptional &operator=(ResettingOptional &&other) noexcept {
    value_ = std::move(other.value_);
    other.value_.reset();
    return *this;
  }
  ~ResettingOptional() = default;

  bool has_value() const { return value_.has_value(); }

  T &value() { return value_.value(); }
  const T &value() const { return value_.value(); }

private:
  std::optional<T> value_;
};

template<typename T>
std::vector<T> takeTail(std::vector<T> *src, size_t maxNumElements) {
  auto count = std::min(src->size(), maxNumElements);
  auto begin = src->end() - count;
  auto end = src->end();
  std::vector<T> result((std::move_iterator(begin)), std::move_iterator(end));
  src->erase(begin, end);
  return result;
}
}  // namespace kosak::coding::misc
