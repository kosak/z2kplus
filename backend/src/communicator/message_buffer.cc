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

#include "z2kplus/backend/communicator/message_buffer.h"

namespace z2kplus::backend::communicator {
namespace internal {
MessageBufferBase::MessageBufferBase() = default;
MessageBufferBase::~MessageBufferBase() = default;

void MessageBufferBase::interrupt() {
  mutex_.lock();
  interrupted_ = true;
  mutex_.unlock();
  condVar_.notify_all();
}

void MessageBufferBase::shutdown() {
  mutex_.lock();
  hasBeenShutdown_ = true;
  mutex_.unlock();
  condVar_.notify_all();
}
}  // namespace internal {
}  // namespace z2kplus::backend::communicator

#if 0
void CommunicatorInternal::appendIncomingMessages(std::vector<ChannelMessage> &&messages) {
  {
    std::scoped_lock<std::mutex> lock(mutex_);
    incoming_.insert(incoming_.end(), std::move_iterator(messages.begin()),
      std::move_iterator(messages.end()));
  }
  cv_.notify_all();
}
void CommunicatorInternal::swapIncomingMessages(int timeoutMillis, vector<ChannelMessage> *result) {
  result->clear();
  std::unique_lock<std::mutex> lock(mutex_);
  cv_.wait_for(lock, std::chrono::milliseconds(timeoutMillis), [this]() { return !incoming_.empty(); });
  result->swap(incoming_);
}
#endif
