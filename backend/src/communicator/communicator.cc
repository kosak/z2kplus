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

#include "z2kplus/backend/communicator/communicator.h"

#include <memory>
#include <string>

#include "kosak/coding/coding.h"
#include "kosak/coding/myjson.h"
#include "kosak/coding/unix.h"
#include "z2kplus/backend/communicator/channel.h"
#include "z2kplus/backend/util/mysocket.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::stringf;
using kosak::coding::ParseContext;
using z2kplus::backend::communicator::MessageBuffer;
using z2kplus::backend::shared::Profile;
using z2kplus::backend::shared::protocol::control::CRequest;
using z2kplus::backend::shared::protocol::control::CResponse;
using z2kplus::backend::shared::protocol::message::DRequest;
using z2kplus::backend::shared::protocol::message::DResponse;
using z2kplus::backend::util::BlockingQueue;
using z2kplus::backend::util::MySocket;

#define HERE KOSAK_CODING_HERE

namespace nsunix = kosak::coding::nsunix;
namespace crequests = z2kplus::backend::shared::protocol::control::crequests;
namespace cresponses = z2kplus::backend::shared::protocol::control::cresponses;

namespace z2kplus::backend::communicator {
namespace {
bool trySendCResponse(CResponse &&response, Channel *channel, const FailFrame &ff);

class MyChannelCallback final : public ChannelCallback {
public:
  explicit MyChannelCallback(std::shared_ptr<MessageBuffer<internal::ChannelMessage>> buffer);
  DISALLOW_COPY_AND_ASSIGN(MyChannelCallback);
  DISALLOW_MOVE_COPY_AND_ASSIGN(MyChannelCallback);
  ~MyChannelCallback() final;

  bool tryOnStartup(Channel *channel, const FailFrame &/*ff*/) final {
    buffer_->append(internal::ChannelMessage(channel->shared_from_this(), true));
    return true;
  }

  bool tryOnShutdown(Channel *channel, const FailFrame &/*ff*/) final {
    buffer_->append(internal::ChannelMessage(channel->shared_from_this(), false));
    return true;
  }

  bool tryOnMessage(Channel *channel, std::string &&message, const FailFrame &ff) final {
    ParseContext ctx(message);
    CRequest req;
    using kosak::coding::tryParseJson;
    if (!tryParseJson(&ctx, &req, ff.nest(HERE))) {
      return false;
    }
    buffer_->append(internal::ChannelMessage(channel->shared_from_this(), std::move(req)));
    return true;
  }

private:
  std::shared_ptr<MessageBuffer<internal::ChannelMessage>> buffer_;
};

bool profileMatches(const Profile &lhs, const Profile &rhs);
}  // namespace

bool Communicator::tryCreate(int requestedPort, std::shared_ptr<CommunicatorCallbacks> callbacks,
    std::shared_ptr<Communicator> *result, const FailFrame &ff) {
  int assignedPort;
  MySocket listenSocket;
  if (!MySocket::tryListen(requestedPort, &assignedPort, &listenSocket, ff.nest(HERE))) {
    return false;
  }
  auto communicator = std::make_shared<Communicator>(Private(), std::move(listenSocket), assignedPort,
      std::move(callbacks));
  std::string listenerThreadName = "Listener";
  std::string processingThreadName = "Processor";
  std::thread listenerThread(&listenerThreadMain, communicator, listenerThreadName);
  std::thread processingThread(&processingThreadMain, communicator, processingThreadName);
  if (!nsunix::trySetThreadName(&listenerThread, listenerThreadName, ff.nest(HERE)) ||
      !nsunix::trySetThreadName(&processingThread, processingThreadName, ff.nest(HERE))) {
    return false;
  }
  listenerThread.detach();
  processingThread.detach();
  *result = std::move(communicator);
  return true;
}

Communicator::Communicator(Private, MySocket &&listenSocket, int listenPort,
    std::shared_ptr<CommunicatorCallbacks> &&callbacks) : listenSocket_(std::move(listenSocket)),
    listenPort_(listenPort), callbacks_(std::move(callbacks)),
    messages_(std::make_shared<MessageBuffer<internal::ChannelMessage>>()) {}
Communicator::~Communicator() = default;

void Communicator::listenerThreadMain(const std::shared_ptr<Communicator> &self,
    const std::string &humanReadablePrefix) {
  kosak::coding::internal::Logger::setThreadPrefix(humanReadablePrefix);
  FailRoot fr;
  if (!self->tryListenForever(humanReadablePrefix, fr.nest(HERE))) {
    warn("listener thread failed: %o", fr);
  }
  warn("Shutting down channels");
  self->shutdownAllChannels();
  warn("listener thread exiting...");
}

void Communicator::processingThreadMain(const std::shared_ptr<Communicator> &self,
    const std::string &humanReadablePrefix) {
  kosak::coding::internal::Logger::setThreadPrefix(humanReadablePrefix);
  FailRoot fr;
  if (!self->tryProcessMessagesForever(humanReadablePrefix, fr.nest(HERE))) {
    warn("listener thread failed: %o", fr);
  }
  warn("Shutting down channels");
  self->shutdownAllChannels();
  warn("listener thread exiting...");
}

bool Communicator::tryListenForever(const std::string &humanReadablePrefix, const FailFrame &ff) {
  // Shared among all active Channels
  auto myCb = std::make_shared<MyChannelCallback>(messages_);
  while (true) {
    MySocket newSocket;
    std::shared_ptr<Channel> channel;
    if (!listenSocket_.tryAccept(&newSocket, ff.nest(HERE)) ||
        !Channel::tryCreate(humanReadablePrefix, std::move(newSocket), myCb, &channel, ff.nest(HERE))) {
      return false;
    }
  }
}

/**
 * This thread owns (has exclusive access to) the data structures in Communicator. So they can be accesssed without
 * any need for synchronization.
 */
bool Communicator::tryProcessMessagesForever(const std::string &humanReadablePrefix, const FailFrame &ff) {
  struct ChannelMessageVisitor {
    ChannelMessageVisitor(Communicator *owner, Channel *channel, const FailFrame *ff) :
        owner_(owner), channel_(channel), ff_(ff) {}

    bool operator()(bool started) const {
      if (started) {
        return owner_->tryHandleChannelStartup(channel_, ff_->nest(HERE));
      }
      return owner_->tryHandleChannelShutdown(channel_, ff_->nest(HERE));
    }

    bool operator()(CRequest &&request) const {
      // Unwrap the request payload.
      return std::visit(*this, std::move(request.payload()));
    }

    // These are for the nested call to visit, if the "outer" message was a CRequest
    bool operator()(crequests::Hello &&hello) const {
      return owner_->tryHandleHello(std::move(hello), channel_, ff_->nest(HERE));
    }
    bool operator()(crequests::CreateSession &&cs) const {
      return owner_->tryHandleCreateSession(std::move(cs), channel_, ff_->nest(HERE));
    }
    bool operator()(crequests::AttachToSession &&as) const {
      return owner_->tryHandleAttachToSession(std::move(as), channel_, ff_->nest(HERE));
    }
    bool operator()(crequests::PackagedRequest &&pr) const {
      return owner_->tryHandlePackagedRequest(std::move(pr), channel_, ff_->nest(HERE));
    }

    Communicator *owner_ = nullptr;
    Channel *channel_ = nullptr;
    const FailFrame *ff_ = nullptr;
  };

  std::vector<internal::ChannelMessage> channelMessages;
  while (true) {
    bool isCancelled;
    messages_->waitForDataAndSwap({}, &channelMessages, &isCancelled);
    if (isCancelled) {
      warn("%o: Message Processor shutting down", humanReadablePrefix);
      return true;
    }

    for (auto &cm : channelMessages) {
      auto ff2 = ff.nest(HERE);
      ChannelMessageVisitor v(this, cm.channel_.get(), &ff2);
      if (!std::visit(v, std::move(cm.payload_))) {
        return false;
      }
    }
  }
}

//void Communicator::unregisterChannel(const std::shared_ptr<Channel> &channel) {
//  std::scoped_lock<std::mutex> guard(mutex_);
//  channels_.erase(channel->id());
//}

void Communicator::shutdownAllChannels() {
  std::cout << "ZAMBONI not threadsafe\n";
//  auto cs = std::move(channels_);
//  channels_.clear();
//  for (auto &c : cs) {
//    c.second->requestShutdown();
//  }
}

bool Communicator::tryHandleChannelStartup(Channel *channel, const FailFrame &/*ff*/) {
  channels_.emplace(channel->id(), channel->shared_from_this());
  return true;
}

bool Communicator::tryHandleChannelShutdown(Channel *channel, const FailFrame &/*ff*/) {
  channels_.erase(channel->id());
  return true;
}

bool Communicator::tryHandleHello(Hello &&hello, Channel *channel, const FailFrame &ff) {
  if (pendingProfiles_.find(channel->id()) != pendingProfiles_.end() ||
      channelToSession_.find(channel->id()) != channelToSession_.end()) {
    return ff.failf(HERE, "Received duplicate Hello message");
  }
  auto profile = std::make_shared<Profile>(std::move(hello.profile()));
  pendingProfiles_.emplace(channel->id(), std::move(profile));
  return true;
}

bool Communicator::tryHandleCreateSession(crequests::CreateSession &&/*createSession*/,
    Channel *channel, const FailFrame &ff) {
  auto node = pendingProfiles_.extract(channel->id());
  if (node.empty()) {
    return ff.failf(HERE, "Can't create session because I never received a Hello");
  }

  auto session = Session::create(std::move(node.mapped()), channel->shared_from_this());
  guidToSession_.emplace(session->guid(), session);

  channelToSession_.emplace(channel->id(), session);

  CResponse resp(cresponses::SessionSuccess(session->guid(), 0, *session->profile()));
  return trySendCResponse(std::move(resp), channel, ff.nest(HERE));
}

bool Communicator::tryHandleAttachToSession(crequests::AttachToSession &&as, Channel *channel, const FailFrame &ff) {
  auto pp = pendingProfiles_.find(channel->id());
  if (pp == pendingProfiles_.end()) {
    return ff.failf(HERE, "Can't attach to session because I never received a Hello");
  }
  auto ip = guidToSession_.find(as.existingSessionGuid());
  if (ip == guidToSession_.end() || !profileMatches(*pp->second, *ip->second->profile())) {
    CResponse resp((cresponses::SessionFailure()));
    return trySendCResponse(std::move(resp), channel, ff.nest(HERE));
  }
  pendingProfiles_.erase(pp);

  const auto &session = ip->second;

  // Associate session with new channel.
  auto formerChannel = session->swapChannel(channel->shared_from_this());

  // Disassociating old channel and shutting it down
  channelToSession_.erase(formerChannel->id());
  formerChannel->requestShutdown();

  // Associating new channel with session, and session with new channel
  channelToSession_.emplace(channel->id(), session);

  CResponse resp(cresponses::SessionSuccess(as.existingSessionGuid(), session->nextExpectedRequestId(),
      *session->profile()));

  return trySendCResponse(std::move(resp), channel, ff.nest(HERE)) &&
      session->tryCatchup(as.nextExpectedResponseId(), channel, ff.nest(HERE));
}

bool Communicator::tryHandlePackagedRequest(crequests::PackagedRequest &&pr, Channel *channel, const FailFrame &ff) {
  auto c2sp = channelToSession_.find(channel->id());
  if (c2sp == channelToSession_.end()) {
    // Stale message from old channel. Session has been bound to a different channel.
    warn("Stale message from old channel -- harmless");
    return true;
  }

  auto &session = c2sp->second;
  if (!session->noteIncomingRequest(pr)) {
    // session says this is a duplicate and we should drop it (we silently do so, without
    // reporting an error).
    warn("Dropping PackagedRequest %o", pr);
    return true;
  }
  return callbacks_->tryOnRequest(session.get(), std::move(pr.request()), ff.nest(HERE));
}

namespace internal {
ChannelMessage::ChannelMessage(std::shared_ptr<Channel> channel, payload_t payload) :
    channel_(std::move(channel)), payload_(std::move(payload)) {}
ChannelMessage::ChannelMessage(ChannelMessage &&) noexcept = default;
ChannelMessage &ChannelMessage::operator=(ChannelMessage &&) noexcept = default;
ChannelMessage::~ChannelMessage() = default;
}  // namespace internal

namespace {
bool trySendCResponse(CResponse &&response, Channel *channel, const FailFrame &ff) {
  std::string text;
  using kosak::coding::tryAppendJson;
  return tryAppendJson(response, &text, ff.nest(HERE)) &&
      channel->trySend(std::move(text), ff.nest(HERE));
}

MyChannelCallback::MyChannelCallback(std::shared_ptr<MessageBuffer<internal::ChannelMessage>> buffer) :
    buffer_(std::move(buffer)) {}
MyChannelCallback::~MyChannelCallback() = default;

bool profileMatches(const Profile &lhs, const Profile &rhs) {
  return lhs.userId() == rhs.userId() && lhs.signature() == rhs.signature();
}
}  // namespace
}  // namespace z2kplus::backend::communicator
