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

#if 0
#include "z2kplus/backend/test/util/client_endpoint.h"

#include <memory>
#include <string>
#include "kosak/coding/failures.h"
#include "kosak/coding/myjson.h"
#include "z2kplus/backend/communicator/channel.h"
#include "z2kplus/backend/communicator/channel_message.h"
#include "z2kplus/backend/communicator/channel_message_builder.h"
#include "z2kplus/backend/coordinator/subscription_response.h"
#include "z2kplus/backend/shared/protocol/controlMiddlewareVsBackend/brequest.h"
#include "z2kplus/backend/shared/protocol/controlMiddlewareVsBackend/bresponse.h"
#include "z2kplus/backend/shared/protocol/message/dresponse.h"
#include "z2kplus/backend/shared/protocol/message/packaging.h"
#include "z2kplus/backend/util/mysocket.h"

using kosak::coding::Failures;
using kosak::coding::ParseContext;
using z2kplus::backend::communicator::Channel;
using z2kplus::backend::communicator::ChannelMessage;
using z2kplus::backend::communicator::ChannelMessageBuilder;
using z2kplus::backend::communicator::MessageBuffer;
using z2kplus::backend::coordinator::SubscriptionResponse;
using z2kplus::backend::util::MySocket;
using z2kplus::backend::shared::protocol::controlMiddlewareVsBackend::BRequest;
using z2kplus::backend::shared::protocol::controlMiddlewareVsBackend::BResponse;
using z2kplus::backend::shared::protocol::message::DResponse;
using z2kplus::backend::shared::protocol::message::PackagedRequest;
using z2kplus::backend::shared::protocol::message::PackagedResponse;
namespace channelMessages = z2kplus::backend::communicator::channelMessages;
namespace brequests = z2kplus::backend::shared::protocol::controlMiddlewareVsBackend::brequests;
namespace bresponses = z2kplus::backend::shared::protocol::controlMiddlewareVsBackend::bresponses;

namespace z2kplus::backend::test::util {

bool ClientEndpoint::tryCreate(const char *server, int port, ClientEndpoint *result,
  Failures *failures) {
  MySocket socket;
  if (!MySocket::tryConnect(server, port, &socket, failures)) {
    return false;
  }

  auto incoming = make_shared<MessageBuffer<ChannelMessage>>();
  std::shared_ptr<Channel> channel;
  if (!Channel::tryCreate(0, std::move(socket), incoming, &channel, failures)) {
    return false;
  }
  *result = ClientEndpoint(std::move(incoming), std::move(channel));
  return true;
}

ClientEndpoint::ClientEndpoint() = default;
ClientEndpoint::ClientEndpoint(std::shared_ptr<MessageBuffer<ChannelMessage>> &&incoming,
  std::shared_ptr<Channel> &&channel) : incoming_(std::move(incoming)),
  channel_(std::move(channel)) {}
ClientEndpoint::ClientEndpoint(ClientEndpoint &&) noexcept = default;
ClientEndpoint &ClientEndpoint::operator=(ClientEndpoint &&) noexcept = default;
ClientEndpoint::~ClientEndpoint() {
  if (channel_ != nullptr) {
    channel_->shutdown();
  }
}

bool ClientEndpoint::tryEstablishSession(Server *server, const string &userId, int timeoutMillis,
  string *sessionId, Failures *failures) {
  auto requestId = nextCreateSessionId_++;
  BRequest breq(brequests::CreateSession(requestId, userId));
  ChannelMessageBuilder<BRequest, PackagedRequest> builder;
  if (!builder.tryAppendControlMessage(breq, failures)) {
    return false;
  }
  channel_->send(&builder);
  auto now = time(nullptr);
  vector<SubscriptionResponse> notifications;
  vector<ChannelMessage> messages;
  bool dummy1, dummy2;
  if (!server->tryRunOnce(now, timeoutMillis, &notifications, &messages, &dummy1, failures) ||
    !tryCheckIncomingMessages(timeoutMillis, &dummy2, failures)) {
    return false;
  }

  auto ip = createdSessions_.find(requestId);
  if (ip == createdSessions_.end()) {
    failures->add("Failed to create session in %o millis", timeoutMillis);
    return false;
  }
  if (ip->second.empty()) {
    failures->add("Session create failed");
    return false;
  }
  *sessionId = std::move(ip->second);
  createdSessions_.erase(ip);
  return true;
}

bool ClientEndpoint::trySendRequest(const std::string &sessionId, DRequest &&request,
  Failures *failures) {
  ChannelMessageBuilder<BRequest, PackagedRequest> builder;
  // crap these stats need to be per-session
  PackagedRequest pr(requestId_, nextExpectedResponseId_, std::move(request));
  if (!builder.tryAppendDataMessage(sessionId, pr, failures)) {
    return false;
  }
  ++requestId_;
  channel_->send(&builder);
  return true;
}

namespace {
class MyVisitor {
public:
  MyVisitor(std::map<std::string, std::vector<DResponse>> *responses,
    map<uint64, string> *createdSessions, Failures *failures) : responses_(responses),
    createdSessions_(createdSessions), failures_(failures) {}
  DISALLOW_COPY_AND_ASSIGN(MyVisitor);
  DISALLOW_MOVE_COPY_AND_ASSIGN(MyVisitor);

  // Channel messages

  bool operator()(channelMessages::Control &&c) {
    BResponse bresp;
    ParseContext ctx(c.message());
    using kosak::coding::tryParseJson;
    return tryParseJson(&ctx, &bresp, failures_) &&
      std::visit(*this, std::move(bresp.payload()));
  }

  bool operator()(channelMessages::Data &&d) {
    PackagedResponse presp;
    ParseContext ctx(d.message());
    using kosak::coding::tryParseJson;
    if (!tryParseJson(&ctx, &presp, failures_)) {
      return false;
    }
    *nextExpectedResponseId_ = presp.responseId() + 1;
    (*responses_)[std::move(d.sessionId())].push_back(std::move(presp.response()));
    return true;
  }

  bool operator()(channelMessages::Shutdown &&s) {
    debug("Channel %o shutting down", s.channel()->id());
    return true;
  }

  // Control responses
  bool operator()(bresponses::SessionSuccess &&ss) {
    if (ss.nextExpectedRequestId() != 0) {
      failures_->add("Expected nextExpectedRequestId == 0, got %o", ss.nextExpectedRequestId());
      return false;
    }
    (*createdSessions_)[ss.requestId()] = std::move(ss.assignedSessionId());
    return true;
  }

  bool operator()(bresponses::SessionFailure &&sf) {
    (*createdSessions_)[sf.requestId()] = "";
    return true;
  }

private:
  // Map from sessionId to vector of responses. Does not own.
  std::map<std::string, std::vector<DResponse>> *responses_;
  // Does not own.
  std::map<uint64, std::string> *createdSessions_;
  // Does not own.
  Failures *failures_;
};
}  // namespace

bool ClientEndpoint::tryCheckIncomingMessages(int timeoutMillis, bool *didSomething,
  Failures *failures) {
  vector<ChannelMessage> messages;
  bool wantShutdown;
  incoming_->waitForDataAndSwap(timeoutMillis, &messages, &wantShutdown);
  if (wantShutdown) {
    failures->add("Unexpected shutdown");
    return false;
  }
  MyVisitor visitor(&responses_, &createdSessions_, failures);
  for (auto &cm : messages) {
    if (!std::visit(visitor, std::move(cm.payload()))) {
      return false;
    }
  }
  *didSomething = !messages.empty();
  return true;
}

const std::vector<DResponse> &ClientEndpoint::peekResponsesFor(const string &sessionId) const {
  const auto ip = responses_.find(sessionId);
  return ip != responses_.end() ? empty_ : ip->second;
}

}  // namespace z2kplus::backend::test::util
#endif
