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

#include <memory>
#include <set>
#include <string>
#include <vector>
#include "kosak/coding/coding.h"
#include "z2kplus/backend/communicator/channel.h"
#include "z2kplus/backend/communicator/message_buffer.h"
#include "z2kplus/backend/communicator/session.h"
#include "z2kplus/backend/shared/protocol/control/crequest.h"
#include "z2kplus/backend/shared/protocol/control/cresponse.h"
#include "z2kplus/backend/shared/protocol/message/drequest.h"
#include "z2kplus/backend/shared/protocol/message/dresponse.h"
#include "z2kplus/backend/shared/protocol/misc.h"
#include "z2kplus/backend/util/blocking_queue.h"
#include "z2kplus/backend/util/mysocket.h"

namespace z2kplus::backend::communicator {
class CommunicatorCallbacks {
protected:
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::shared::protocol::message::DRequest DRequest;

public:
  CommunicatorCallbacks() = default;
  DISALLOW_COPY_AND_ASSIGN(CommunicatorCallbacks);
  DISALLOW_MOVE_COPY_AND_ASSIGN(CommunicatorCallbacks);
  virtual ~CommunicatorCallbacks() = default;

  virtual bool tryOnRequest(Session *session, DRequest &&message, const FailFrame &ff) = 0;
};

namespace internal {
struct ChannelMessage final {
  // startup/shutdown (true/false) or CRequest
  typedef std::variant<bool, z2kplus::backend::shared::protocol::control::CRequest> payload_t;

  ChannelMessage(std::shared_ptr<Channel> channel, payload_t payload);

  DISALLOW_COPY_AND_ASSIGN(ChannelMessage);
  DECLARE_MOVE_COPY_AND_ASSIGN(ChannelMessage);
  ~ChannelMessage();

  std::shared_ptr<Channel> channel_;
  payload_t payload_;
};
}  // namespace internal

class Communicator {
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::util::MySocket MySocket;
  typedef z2kplus::backend::shared::Profile Profile;
  typedef z2kplus::backend::shared::protocol::message::DRequest DRequest;
  typedef z2kplus::backend::shared::protocol::message::DResponse DResponse;
  typedef z2kplus::backend::shared::protocol::control::CResponse CResponse;
  typedef z2kplus::backend::shared::protocol::control::crequests::AttachToSession AttachToSession;
  typedef z2kplus::backend::shared::protocol::control::crequests::CreateSession CreateSession;
  typedef z2kplus::backend::shared::protocol::control::crequests::Hello Hello;
  typedef z2kplus::backend::shared::protocol::control::crequests::PackagedRequest PackagedRequest;

  struct Private {};

public:
  static bool tryCreate(int requestedPort, std::shared_ptr<CommunicatorCallbacks> callbacks,
      std::shared_ptr<Communicator> *result, const FailFrame &ff);

  Communicator(Private, MySocket &&listenSocket, int listenPort,
      std::shared_ptr<CommunicatorCallbacks> &&callbacks);
  DISALLOW_COPY_AND_ASSIGN(Communicator);
  DISALLOW_MOVE_COPY_AND_ASSIGN(Communicator);
  ~Communicator();

  [[nodiscard]] int listenPort() const {
    return listenPort_;
  }

  void shutdown() {
    listenSocket_.close();
  }

  bool trySendResponse(Session *session, DResponse &&response, const FailFrame &ff);

private:
  static void listenerThreadMain(const std::shared_ptr<Communicator> &self,
      const std::string &threadName);
  static void processingThreadMain(const std::shared_ptr<Communicator> &self,
      const std::string &threadName);

  bool tryListenForever(const std::string &humanReadablePrefix, const FailFrame &ff);

  bool tryProcessMessagesForever(const std::string &humanReadablePrefix, const FailFrame &ff);

  void shutdownAllChannels();

  bool tryHandleChannelStartup(Channel *channel, const FailFrame &ff);
  bool tryHandleChannelShutdown(Channel *channel, const FailFrame &ff);

  bool tryHandleHello(Hello &&hello, Channel *channel, const FailFrame &ff);
  bool tryHandleCreateSession(CreateSession &&cs, Channel *channel, const FailFrame &ff);
  bool tryHandleAttachToSession(AttachToSession &&as, Channel *channel,
      const FailFrame &ff);
  bool tryHandlePackagedRequest(PackagedRequest &&pr, Channel *channel, const FailFrame &ff);

  MySocket listenSocket_;
  int listenPort_ = 0;
  std::shared_ptr<CommunicatorCallbacks> callbacks_;
  std::shared_ptr<MessageBuffer<internal::ChannelMessage>> messages_;
  std::map<channelId_t, std::shared_ptr<Channel>> channels_;
  std::map<std::string, std::shared_ptr<Session>> guidToSession_;
  std::map<channelId_t, std::shared_ptr<Profile>> pendingProfiles_;
  std::map<channelId_t, std::shared_ptr<Session>> channelToSession_;
};
}  // namespace z2kplus::backend::events
