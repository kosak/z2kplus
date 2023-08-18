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
#include "z2kplus/backend/coordinator/coordinator.h"
#include "z2kplus/backend/coordinator/subscription.h"
#include "z2kplus/backend/shared/protocol/control/crequest.h"
#include "z2kplus/backend/shared/protocol/control/cresponse.h"
#include "z2kplus/backend/shared/protocol/message/drequest.h"
#include "z2kplus/backend/shared/protocol/message/dresponse.h"

namespace z2kplus::backend::communicator {
namespace internal {
class Robustifier {
  typedef kosak::coding::FailFrame FailFrame;

  template<typename R, typename ...ARGS>
  using Delegate = kosak::coding::Delegate<R, ARGS...>;

public:
  Robustifier(uint64_t nextOutgoingId, uint64_t nextExpectedIncomingId);
  ~Robustifier();

  bool trySend(const Delegate<bool, uint64_t, uint64_t, std::string *, const FailFrame &> &cb,
      Channel *channel, const FailFrame &ff);

  bool noteIncoming(uint64_t incomingId, uint64_t nextExpectedOutgoingId);

  bool tryCatchup(uint64_t nextExpectedOutgoingId, Channel *channel, const FailFrame &ff);

  uint64_t nextExpectedIncomingId() const { return nextExpectedIncomingId_; }

private:
  uint64_t nextOutgoingId_ = 0;
  uint64_t nextExpectedIncomingId_ = 1000;
  std::deque<std::pair<uint64_t, std::string>> unacknowledgedOutgoing_;
};
}  // namespace internal

// A Robustifier from the point of view of the frontend, sending requests to a backend, and receiving responses.
// This is included in the codebase for the purpose of making a fake frontend for tests.
class FrontendRobustifier {
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::shared::protocol::control::cresponses::PackagedResponse PackagedResponse;
  typedef z2kplus::backend::shared::protocol::message::DRequest DRequest;
public:
  FrontendRobustifier();

  bool trySendRequest(DRequest &&request, Channel *channel, const FailFrame &ff);

  bool noteIncomingResponse(const PackagedResponse &pr) {
    return rb_.noteIncoming(pr.responseId(), pr.nextExpectedRequestId());
  }

  bool tryCatchup(size_t nextExpectedRequestId, Channel *channel, const FailFrame &ff) {
    return rb_.tryCatchup(nextExpectedRequestId, channel, ff.nest(KOSAK_CODING_HERE));
  }

  uint64_t nextExpectedResponseId() {
    return rb_.nextExpectedIncomingId();
  }

private:
  internal::Robustifier rb_;
};

// A Robustifier from the point of view of the backend, receiving requests from a frontend (i.e. the client), and
// sending responses.
class BackendRobustifier {
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::shared::protocol::control::crequests::PackagedRequest PackagedRequest;
  typedef z2kplus::backend::shared::protocol::message::DRequest DRequest;
  typedef z2kplus::backend::shared::protocol::message::DResponse DResponse;
public:
  BackendRobustifier();

  bool trySendResponse(DResponse &&response, Channel *channel, const FailFrame &ff);

  bool noteIncoming(const PackagedRequest &pr) {
    return rb_.noteIncoming(pr.requestId(), pr.nextExpectedResponseId());
  }

  bool tryCatchup(size_t nextExpectedResponseId, Channel *channel, const FailFrame &ff) {
    return rb_.tryCatchup(nextExpectedResponseId, channel, ff.nest(KOSAK_CODING_HERE));
  }

  uint64_t nextExpectedRequestId() {
    return rb_.nextExpectedIncomingId();
  }

private:
  internal::Robustifier rb_;
};
}  // namespace z2kplus::backend::communicator
