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

#include <any>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/delegate.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/myjson.h"
#include "kosak/coding/strongint.h"
#include "z2kplus/backend/util/mysocket.h"
#include "z2kplus/backend/communicator/message_buffer.h"
#include "z2kplus/backend/shared/protocol/misc.h"

namespace z2kplus::backend::communicator {
namespace internal {
struct ChannelIdTag {
  static constexpr const char text[] = "ChannelId";
};
}  // namespace internal
typedef kosak::coding::StrongInt<uint64_t, internal::ChannelIdTag> channelId_t;

class Channel;
class ChannelCallback {
protected:
  typedef kosak::coding::FailFrame FailFrame;

public:
  ChannelCallback() = default;
  DISALLOW_COPY_AND_ASSIGN(ChannelCallback);
  DISALLOW_MOVE_COPY_AND_ASSIGN(ChannelCallback);
  virtual ~ChannelCallback() = default;

  virtual bool tryOnStartup(Channel *channel, const FailFrame &ff) = 0;
  virtual bool tryOnMessage(Channel *channel, std::string &&message, const FailFrame &ff) = 0;
  virtual bool tryOnShutdown(Channel *channel, const FailFrame &ff) = 0;
};

class Channel : public std::enable_shared_from_this<Channel> {
  struct Private {};

  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::util::MySocket MySocket;

  template<typename R, typename ...ARGS>
  using Delegate = kosak::coding::Delegate<R, ARGS...>;

public:
  static bool tryCreate(std::string humanReadablePrefix, MySocket socket,
      std::shared_ptr<ChannelCallback> callbacks, std::shared_ptr<Channel> *result, const FailFrame &ff);
  Channel(Private, channelId_t channelId, std::string humanReadablePrefix, MySocket socket,
      std::shared_ptr<ChannelCallback> callbacks);
  DISALLOW_COPY_AND_ASSIGN(Channel);
  DISALLOW_MOVE_COPY_AND_ASSIGN(Channel);
  ~Channel();

  bool trySend(std::string message, const FailFrame &ff);

  /**
   * Request a shutdown. Does nothing if the shutdown has already been requested.
   */
  void requestShutdown();

  channelId_t id() const { return id_; }

private:
  static void readerThreadMain(const std::shared_ptr<Channel> &self, std::string logPrefix);
  static void writerThreadMain(const std::shared_ptr<Channel> &self, std::string logPrefix);

  bool runReaderThreadForever(const FailFrame &ff);
  bool runWriterThreadForever(const FailFrame &ff);
  void maybeTransmitShutdownMessage();

  static std::atomic<uint64_t> nextFreeId_;

  channelId_t id_;
  std::string humanReadablePrefix_;
  MySocket socket_;
  std::shared_ptr<ChannelCallback> callbacks_;

  /**
   * Number of alive threads (reader + writer). When this reaches 0, the last one out sends the "goodbye" message.
   */
  std::atomic<size_t> numThreadsAlive_ = 2;

  std::mutex mutex_;
  std::condition_variable condVar_;
  std::string outgoing_;
  bool shutdownRequested_ = false;
};

class ChannelMultiBuilder {
public:
  std::string *startNextCommand() {
    if (!buffer_.empty()) {
      buffer_.push_back('\n');
    }
    return &buffer_;
  }
  std::string releaseBuffer() { return std::move(buffer_); }

private:
  std::string buffer_;
};
} // namespace z2kplus::backend::communicator
