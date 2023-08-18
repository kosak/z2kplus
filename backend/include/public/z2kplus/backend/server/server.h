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
#include <map>
#include <memory>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/myjson.h"
#include "z2kplus/backend/communicator/channel.h"
#include "z2kplus/backend/communicator/communicator.h"
#include "z2kplus/backend/coordinator/coordinator.h"
#include "z2kplus/backend/coordinator/subscription.h"
#include "z2kplus/backend/shared/protocol/message/drequest.h"
#include "z2kplus/backend/shared/protocol/misc.h"

namespace z2kplus::backend::server {

namespace internal {
class ServerCallbacks;

struct SessionAndDRequest {
  typedef z2kplus::backend::communicator::Session Session;
  typedef z2kplus::backend::shared::protocol::message::DRequest DRequest;

  SessionAndDRequest(std::shared_ptr<Session> session, DRequest request);
  DISALLOW_COPY_AND_ASSIGN(SessionAndDRequest);
  DECLARE_MOVE_COPY_AND_ASSIGN(SessionAndDRequest);
  ~SessionAndDRequest();

  std::shared_ptr<Session> session_;
  DRequest request_;
};

struct ReindexingState {
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::files::DateAndPartKey DateAndPartKey;
  typedef z2kplus::backend::files::FileKey FileKey;
  typedef z2kplus::backend::files::PathMaster PathMaster;

  template<typename T>
  using MessageBuffer = z2kplus::backend::communicator::MessageBuffer<T>;

  static bool tryCreate(std::shared_ptr<PathMaster> pm,
      std::shared_ptr<MessageBuffer<internal::SessionAndDRequest>> todo,
      DateAndPartKey filesEndKey, DateAndPartKey unloggedBeginKeyAfterPurge,
      std::shared_ptr<ReindexingState> *result, const FailFrame &ff);

  ReindexingState(std::shared_ptr<PathMaster> pm,
      std::shared_ptr<MessageBuffer<internal::SessionAndDRequest>> todo,
      DateAndPartKey filesEndKey,
      DateAndPartKey newUnloggedBeginKey);

  static void run(std::shared_ptr<ReindexingState> self);

  bool tryRunHelper(const FailFrame &ff);
  bool tryCleanup(const FailFrame &ff);

  std::shared_ptr<PathMaster> pm_;
  std::shared_ptr<MessageBuffer<internal::SessionAndDRequest>> todo_;
  DateAndPartKey filesEndKey_;
  DateAndPartKey unloggedBeginKeyAfterPurge_;
  std::atomic<bool> done_ = false;
  std::thread activeThread_;
  std::string error_;
};
}  // namespace internal

class Server {
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::communicator::Channel Channel;
  typedef z2kplus::backend::communicator::Communicator Communicator;
  typedef z2kplus::backend::communicator::Session Session;
  typedef z2kplus::backend::communicator::sessionId_t sessionId_t;
  typedef z2kplus::backend::coordinator::Coordinator Coordinator;
  typedef z2kplus::backend::coordinator::Coordinator::response_t coordinatorResponse_t;
  typedef z2kplus::backend::coordinator::Subscription Subscription;
  typedef z2kplus::backend::coordinator::subscriptionId_t subscriptionId_t;
  typedef z2kplus::backend::shared::Profile Profile;
  typedef z2kplus::backend::shared::protocol::message::DRequest DRequest;
  typedef z2kplus::backend::shared::protocol::message::DResponse DResponse;
  typedef z2kplus::backend::shared::protocol::message::drequests::Subscribe SubscribeRequest;

  template<typename T>
  using MessageBuffer = z2kplus::backend::communicator::MessageBuffer<T>;

  template<typename R, typename ...Args>
  using Delegate = kosak::coding::Delegate<R, Args...>;

  struct Private {};

  static constexpr const char serverName[] = "Server";

public:
  static bool tryCreate(Coordinator &&coordinator, int requestedPort,
      std::shared_ptr<Server> *result, const FailFrame &ff);
  Server();
  Server(Private, std::shared_ptr<Communicator> communicator, Coordinator coordinator,
      std::shared_ptr<Profile> adminProfile,
      std::shared_ptr<MessageBuffer<internal::SessionAndDRequest>> todo,
      std::chrono::system_clock::time_point nextPurgeTime,
      std::chrono::system_clock::time_point nextReindexingTime);
  DISALLOW_COPY_AND_ASSIGN(Server);
  DISALLOW_MOVE_COPY_AND_ASSIGN(Server);
  ~Server();

  [[nodiscard]] const Coordinator &coordinator() const { return coordinator_; }

  [[nodiscard]] int listenPort() const {
    return communicator_->listenPort();
  }

  bool tryStop(const FailFrame &ff);

private:
  static void threadMain(const std::shared_ptr<Server> &self);
  bool tryRunForever(const FailFrame &ff);
  bool tryManageReindexing(std::chrono::system_clock::time_point now,
      std::vector<std::string> *statusMessages, const FailFrame &ff);
  bool tryManagePurging(std::chrono::system_clock::time_point now,
      std::vector<std::string> *statusMessages, const FailFrame &ff);

  bool tryProcessRequests(std::chrono::system_clock::time_point now,
      std::vector<internal::SessionAndDRequest> incomingBuffer,
      const FailFrame &ff);
  bool tryProcessResponses(std::vector<coordinatorResponse_t> responses,
      Session *optionalSenderSession, const FailFrame &ff);

  void handleRequest(DRequest &&req, Session *session, std::chrono::system_clock::time_point now,
      std::vector<coordinatorResponse_t> *responses);
  void handleNonSubRequest(DRequest &&req, Subscription *sub,
      std::chrono::system_clock::time_point now, std::vector<coordinatorResponse_t> *responses);
  void handleSubscribeRequest(SubscribeRequest &&subReq, Session *session,
      std::vector<coordinatorResponse_t> *responses);

  std::shared_ptr<Communicator> communicator_;
  Coordinator coordinator_;
  std::shared_ptr<Profile> adminProfile_;
  std::shared_ptr<MessageBuffer<internal::SessionAndDRequest>> todo_;
  std::chrono::system_clock::time_point nextPurgeTime_;
  std::chrono::system_clock::time_point nextReindexingTime_;
  std::map<sessionId_t, std::shared_ptr<Subscription>> sessionToSubscription_;
  std::map<subscriptionId_t, std::shared_ptr<Session>> subscriptionToSession_;
  std::shared_ptr<internal::ReindexingState> reindexingState_;

  friend class internal::ServerCallbacks;
};
}  // namespace z2kplus::backend::server
