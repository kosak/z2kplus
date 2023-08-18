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

#if 0
#include <memory>
#include <string>
#include <vector>
#include "kosak/coding/failures.h"
#include "z2kplus/backend/communicator/channel.h"
#include "z2kplus/backend/communicator/channel_message.h"
#include "z2kplus/backend/communicator/message_buffer.h"
#include "z2kplus/backend/communicator/communicator.h"
#include "z2kplus/backend/server/server.h"
#include "z2kplus/backend/shared/protocol/message/drequest.h"
#include "z2kplus/backend/shared/protocol/message/dresponse.h"
#include "z2kplus/backend/util/mysocket.h"

namespace z2kplus::backend::test::util {
class ClientEndpoint {
  typedef z2kplus::backend::communicator::Channel Channel;
  typedef z2kplus::backend::communicator::Communicator Communicator;
  typedef z2kplus::backend::communicator::ChannelMessage ChannelMessage;
  typedef z2kplus::backend::server::Server Server;
  typedef z2kplus::backend::shared::protocol::message::DRequest DRequest;
  typedef z2kplus::backend::shared::protocol::message::DResponse DResponse;
  typedef z2kplus::backend::util::MySocket MySocket;
  typedef kosak::coding::Failures Failures;
  template<typename T>
  using MessageBuffer = z2kplus::backend::communicator::MessageBuffer<T>;

public:
  static bool tryCreate(const char *server, int port, ClientEndpoint *result, Failures *failures);

  ClientEndpoint();
  DISALLOW_COPY_AND_ASSIGN(ClientEndpoint);
  DECLARE_MOVE_COPY_AND_ASSIGN(ClientEndpoint);
  ~ClientEndpoint();

  bool tryEstablishSession(Server *server, const std::string &userId, int timeoutMillis,
    std::string *sessionId, Failures *failures);

  bool trySendRequest(const std::string &sessionId, DRequest &&request, Failures *failures);
  bool tryCheckIncomingMessages(int timeoutMillis, bool *didSomething, Failures *failures);

  const std::vector<DResponse> &peekResponsesFor(const std::string &sessionId) const;

private:
  ClientEndpoint(std::shared_ptr<MessageBuffer<ChannelMessage>> &&incoming,
    std::shared_ptr<Channel> &&channel);

  std::shared_ptr<MessageBuffer<ChannelMessage>> incoming_;
  std::shared_ptr<Channel> channel_;
  uint64 nextCreateSessionId_ = 0;
  std::map<uint64, std::string> createdSessions_;
  // sessionId -> responses
  std::map<std::string, std::vector<DResponse>> responses_;
  // empty vector for use of peekResponsesFor
  std::vector<DResponse> empty_;
};

}  // namespace z2kplus::backend::test::util
#endif
