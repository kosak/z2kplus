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

#include <deque>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/strongint.h"
#include "z2kplus/backend/communicator/channel.h"
#include "z2kplus/backend/communicator/robustifier.h"
#include "z2kplus/backend/coordinator/coordinator.h"
#include "z2kplus/backend/coordinator/subscription.h"
#include "z2kplus/backend/shared/profile.h"
#include "z2kplus/backend/shared/protocol/control/crequest.h"
#include "z2kplus/backend/shared/protocol/control/cresponse.h"
#include "z2kplus/backend/shared/protocol/message/drequest.h"
#include "z2kplus/backend/shared/protocol/message/dresponse.h"

namespace z2kplus::backend::communicator {
namespace internal {
struct SessionlIdTag {
  static constexpr const char text[] = "SessionId";
};
}  // namespace internal
typedef kosak::coding::StrongInt<uint64_t, internal::SessionlIdTag> sessionId_t;

class Session : public std::enable_shared_from_this<Session> {
  struct Private {};
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::coordinator::Coordinator Coordinator;
  typedef z2kplus::backend::coordinator::Subscription Subscription;
  typedef z2kplus::backend::shared::Profile Profile;
  typedef z2kplus::backend::shared::protocol::control::crequests::PackagedRequest PackagedRequest;
  typedef z2kplus::backend::shared::protocol::control::cresponses::PackagedResponse PackagedResponse;
  typedef z2kplus::backend::shared::protocol::message::DRequest DRequest;
  typedef z2kplus::backend::shared::protocol::message::DResponse DResponse;

  template<typename R, typename ...Args>
  using Delegate = kosak::coding::Delegate<R, Args...>;

public:
  static std::shared_ptr<Session> create(std::shared_ptr<Profile> profile, std::shared_ptr<Channel> channel);

  Session(Private, sessionId_t id, std::string &&guid, std::shared_ptr<Profile> &&profile,
      std::shared_ptr<Channel> &&channel);
  DISALLOW_COPY_AND_ASSIGN(Session);
  DISALLOW_MOVE_COPY_AND_ASSIGN(Session);
  ~Session();

  bool tryCatchup(uint64_t nextExpectedResponseId, Channel *channel, const FailFrame &ff) {
    return rb_.tryCatchup(nextExpectedResponseId, channel, ff.nest(KOSAK_CODING_HERE));
  }

  bool noteIncomingRequest(const PackagedRequest &pr) {
    return rb_.noteIncoming(pr);
  }

  bool trySendResponse(DResponse &&response, const FailFrame &ff) {
    return rb_.trySendResponse(std::move(response), channel_.get(), ff.nest(KOSAK_CODING_HERE));
  }

  std::shared_ptr<Channel> swapChannel(std::shared_ptr<Channel> newChannel);

  uint64_t nextExpectedRequestId() {
    return rb_.nextExpectedRequestId();
  }

  sessionId_t id() const { return id_; }
  const std::string &guid() const { return guid_; }
  const std::shared_ptr<Profile> &profile() const { return profile_; }

  const std::chrono::system_clock::time_point &lastActivityTime() const { return lastActivityTime_; }
  std::chrono::system_clock::time_point lastActivityTime() { return lastActivityTime_; }

private:
  sessionId_t id_;
  std::string guid_;
  std::shared_ptr<Profile> profile_;
  std::shared_ptr<Channel> channel_;

  BackendRobustifier rb_;

  bool shutdown_ = false;

  // For inactivity timeout
  std::chrono::system_clock::time_point lastActivityTime_ = {};
};
}  // namespace z2kplus::backend::communicator
