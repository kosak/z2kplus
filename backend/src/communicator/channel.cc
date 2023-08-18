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

#include "z2kplus/backend/communicator/channel.h"

#include <optional>
#include <string_view>
#include <thread>
#include <utility>
#include "kosak/coding/coding.h"
#include "kosak/coding/text/conversions.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::text::tryParseDecimal;

#define HERE KOSAK_CODING_HERE

namespace nsunix = kosak::coding::nsunix;

namespace z2kplus::backend::communicator {
std::atomic<uint64_t> Channel::nextFreeId_ = 0;

bool Channel::tryCreate(std::string humanReadablePrefix, MySocket socket,
    std::shared_ptr<ChannelCallback> callbacks,
    std::shared_ptr<Channel> *result, const FailFrame &ff) {
  auto id = channelId_t(nextFreeId_++);
  auto channel = std::make_shared<Channel>(Private(), id, std::move(humanReadablePrefix),
      std::move(socket), std::move(callbacks));
  std::string readerLogName = stringf("r-%o-%o", humanReadablePrefix, id);
  std::string writerLogName = stringf("w-%o-%o", humanReadablePrefix, id);
  std::thread rt(&readerThreadMain, channel, readerLogName);
  std::thread wt(&writerThreadMain, channel, writerLogName);

  std::string readerThreadName = stringf("rchan%o", id).substr(0, 15);
  std::string writerThreadName = stringf("wchan%o", id).substr(0, 15);
  if (!nsunix::trySetThreadName(&rt, readerThreadName, ff.nest(HERE)) ||
      !nsunix::trySetThreadName(&wt, writerThreadName, ff.nest(HERE))) {
    // If this ever fails it will actually crash because we haven't detached the threads.
    // Whatever. Don't care.
    return false;
  }
  rt.detach();
  wt.detach();
  *result = std::move(channel);
  return (*result)->callbacks_->tryOnStartup(result->get(), ff.nest(HERE));
}

Channel::Channel(Private, channelId_t id, std::string humanReadablePrefix, MySocket socket,
    std::shared_ptr<ChannelCallback> callbacks) : id_(id), humanReadablePrefix_(std::move(humanReadablePrefix)),
    socket_(std::move(socket)), callbacks_(std::move(callbacks)) {}
Channel::~Channel() = default;

bool Channel::trySend(std::string message, const FailFrame &/*ff*/) {
  if (message.empty()) {
    return true;
  }
  std::unique_lock guard(mutex_);
  auto needsNotify = outgoing_.empty();
  if (needsNotify) {
    outgoing_ = std::move(message);
  } else {
    outgoing_.append(message);
  }
  outgoing_.push_back('\n');
  guard.unlock();
  if (needsNotify) {
    condVar_.notify_all();
  }
  return true;
}

void Channel::requestShutdown() {
  std::unique_lock guard(mutex_);
  if (shutdownRequested_) {
    return;
  }
  shutdownRequested_ = true;
  guard.unlock();
  // This should unblock the read thread and cause it to have an error.
  socket_.close();
  condVar_.notify_all();
}

void Channel::readerThreadMain(const std::shared_ptr<Channel> &self, std::string logPrefix) {
  kosak::coding::internal::Logger::setThreadPrefix(std::move(logPrefix));
  warn("Channel %o: Reader thread starting", self->id_);
  {
    FailRoot fr;
    if (!self->runReaderThreadForever(fr.nest(HERE))) {
      warn("Channel %o: reader thread failed: %o", self->id_, fr);
    }
  }

  self->requestShutdown();
  self->maybeTransmitShutdownMessage();
  warn("Channel %o: reader thread exiting...", self->id_);
}

void Channel::writerThreadMain(const std::shared_ptr<Channel> &self, std::string logPrefix) {
  kosak::coding::internal::Logger::setThreadPrefix(std::move(logPrefix));
  warn("Channel %o: Writer thread starting", self->id_);
  FailRoot fr;
  if (!self->runWriterThreadForever(fr.nest(HERE))) {
    warn("Channel %o: writer thread failed: %o", self->id_, fr);
  }
  self->requestShutdown();
  self->maybeTransmitShutdownMessage();
  warn("Channel %o: writer thread exiting...", self->id_);
}

void Channel::maybeTransmitShutdownMessage() {
  if (--numThreadsAlive_ != 0) {
    return;
  }
  FailRoot fr(true);
  if (!callbacks_->tryOnShutdown(this, fr.nest(HERE))) {
    warn("Channel %o: callback reported failure on shutdown (ignoring)...", id_);
  }
}

class Chunker {
public:
  void push(std::string_view fragment) {
    buffer_.append(fragment);
  }

  std::optional<std::string_view> maybePop();

private:
  std::string buffer_;
  size_t nextStart_ = 0;
};

std::optional<std::string_view> Chunker::maybePop() {
  auto terminatorPos = buffer_.find('\n', nextStart_);
  if (terminatorPos == std::string::npos) {
    // No record terminator in current buffer. Remove any prefix material and return failure.
    buffer_.erase(0, nextStart_);
    nextStart_ = 0;
    return {};
  }
  auto result = std::string_view(buffer_.data() + nextStart_, terminatorPos - nextStart_);
  debug("result is %o", result);
  nextStart_ = terminatorPos + 1;
  return result;
}

bool Channel::runReaderThreadForever(const FailFrame &ff) {
  Chunker chunker;
  std::array<char, 4096> buffer = {};
  while (true) {
    size_t bytesRead;
    if (!nsunix::tryRead(socket_.fd(), buffer.data(), buffer.size(), &bytesRead, ff.nest(HERE))) {
      return false;
    }

    if (bytesRead == 0) {
      return true;
    }

    std::string_view fragment(buffer.data(), bytesRead);
    chunker.push(fragment);

    while (true) {
      auto message = chunker.maybePop();
      if (!message.has_value()) {
        break;
      }
      if (message->empty()) {
        continue;
      }
      debug("%o: received %o", id_, *message);
      std::string messageString(*message);
      if (!callbacks_->tryOnMessage(this, std::move(messageString), ff.nest(HERE))) {
        return false;
      }
    }
  }
}

bool Channel::runWriterThreadForever(const FailFrame &ff) {
  std::unique_lock guard(mutex_);
  std::string localBuffer;
  while (true) {
    while (true) {
      if (shutdownRequested_) {
        warn("Writer thread shutdown requested");
        return true;
      }
      if (!outgoing_.empty()) {
        break;
      }
      condVar_.wait(guard);
    }
    localBuffer.swap(outgoing_);
    guard.unlock();

    debug("%o: writing %o", id_, localBuffer);
    if (!nsunix::tryWriteAll(socket_.fd(), localBuffer.data(), localBuffer.size(), ff.nest(HERE))) {
      return false;
    }

    localBuffer.clear();
    guard.lock();
  }
}
} // namespace z2kplus::backend::communicator
