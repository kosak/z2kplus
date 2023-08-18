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
#include <condition_variable>
#include <mutex>
#include <vector>

#include "kosak/coding/misc.h"

namespace z2kplus::backend::communicator {
namespace internal {
class MessageBufferBase {
public:
  MessageBufferBase();
  DISALLOW_COPY_AND_ASSIGN(MessageBufferBase);
  DISALLOW_MOVE_COPY_AND_ASSIGN(MessageBufferBase);
  ~MessageBufferBase();

  void interrupt();
  void shutdown();

protected:
  std::mutex mutex_;
  std::condition_variable condVar_;
  bool interrupted_ = false;
  bool hasBeenShutdown_ = false;
};
}  // namespace internal

template<typename T>
class MessageBuffer : public internal::MessageBufferBase {
  typedef kosak::coding::misc::WaitValueResult WaitValueResult;
public:
  MessageBuffer();
  DISALLOW_COPY_AND_ASSIGN(MessageBuffer);
  DISALLOW_MOVE_COPY_AND_ASSIGN(MessageBuffer);
  ~MessageBuffer();

  void append(T item);
  void appendVec(std::vector<T> vec);

  /**
   * @param timeoutMillis: unset means forever, 0 means return immediately, otherwise wait that long
   */
  void waitForDataAndSwap(std::optional<std::chrono::milliseconds> timeout, std::vector<T> *result,
      bool *wantShutdown);

private:
  std::vector<T> buffer_;
};

template<typename T>
MessageBuffer<T>::MessageBuffer() = default;

template<typename T>
MessageBuffer<T>::~MessageBuffer() = default;

template<typename T>
void MessageBuffer<T>::append(T item) {
  mutex_.lock();
  buffer_.push_back(std::move(item));
  mutex_.unlock();
  condVar_.notify_all();
}

template<typename T>
void MessageBuffer<T>::appendVec(std::vector<T> vec) {
  mutex_.lock();
  buffer_.insert(buffer_.end(), std::move_iterator(vec.begin()), std::move_iterator(vec.end()));
  mutex_.unlock();
  condVar_.notify_all();
}

template<typename T>
void MessageBuffer<T>::waitForDataAndSwap(std::optional<std::chrono::milliseconds> timeout,
    std::vector<T> *result, bool *wantShutdown) {
  result->clear();
  auto waitPoll = [this, result] {
    if (hasBeenShutdown_) {
      return WaitValueResult::Cancelled;
    }
    if (interrupted_ || !buffer_.empty()) {
      interrupted_ = false;
      result->swap(buffer_);
      return WaitValueResult::Ready;
    }
    return WaitValueResult::Timeout;
  };
  auto wvr = kosak::coding::misc::waitForLogic(&mutex_, &condVar_, waitPoll, timeout);
  *wantShutdown = wvr == WaitValueResult::Cancelled;
}
}  // namespace z2kplus::backend::communicator
