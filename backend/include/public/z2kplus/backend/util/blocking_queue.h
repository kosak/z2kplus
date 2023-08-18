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

#include <chrono>
#include <condition_variable>
#include <list>
#include <memory>
#include <thread>
#include "kosak/coding/coding.h"

namespace z2kplus::backend::util {

namespace internal {
class BlockingQueueBase {
public:
  BlockingQueueBase();
  DISALLOW_COPY_AND_ASSIGN(BlockingQueueBase);
  DISALLOW_MOVE_COPY_AND_ASSIGN(BlockingQueueBase);
  ~BlockingQueueBase();

protected:
  std::mutex mutex_;
  std::condition_variable cond_;
};
}  // namespace internal

template<typename T>
class BlockingQueue : public internal::BlockingQueueBase {
public:
  BlockingQueue();
  ~BlockingQueue();
  void push(T &&element);
  T pop();
  bool tryPop(T *result, uint32 timeoutSecs);

private:
  std::list<T> events_;
};

template<typename T>
BlockingQueue<T>::BlockingQueue() = default;
template<typename T>
BlockingQueue<T>::~BlockingQueue() = default;

template<typename T>
void BlockingQueue<T>::push(T &&element) {
  mutex_.lock();
  events_.push_back(std::forward<T>(element));
  mutex_.unlock();
  // C++ wants us to notify outside the lock
  cond_.notify_all();
}

template<typename T>
T BlockingQueue<T>::pop() {
  std::unique_lock<std::mutex> lock(mutex_);
  while (events_.empty()) {
    cond_.wait(lock);
  }
  T result = std::move(events_.front());
  events_.pop_front();
  return result;
}

template<typename T>
bool BlockingQueue<T>::tryPop(T *result, uint32 timeoutSecs) {
  std::unique_lock<std::mutex> lock(mutex_);
  while (events_.empty()) {
    auto waitResult = cond_.wait_for(lock, std::chrono::seconds(timeoutSecs));
    if (waitResult == std::cv_status::timeout) {
      return false;
    }
  }
  *result = std::move(events_.front());
  events_.pop_front();
  return true;
}


}  // namespace z2kplus::backend::util
