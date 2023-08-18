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

#include "z2kplus/backend/communicator/robustifier.h"

#include "kosak/coding/coding.h"
#include "kosak/coding/delegate.h"
#include "kosak/coding/failures.h"

using z2kplus::backend::shared::protocol::control::CRequest;
using z2kplus::backend::shared::protocol::control::CResponse;
namespace crequests = z2kplus::backend::shared::protocol::control::crequests;
namespace cresponses = z2kplus::backend::shared::protocol::control::cresponses;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::communicator {
namespace internal {
Robustifier::Robustifier(uint64_t nextOutgoingId, uint64_t nextExpectedIncomingId) :
    nextOutgoingId_(nextOutgoingId), nextExpectedIncomingId_(nextExpectedIncomingId) {}
Robustifier::~Robustifier() = default;

bool Robustifier::trySend(const Delegate<bool, uint64_t, uint64_t, std::string *, const FailFrame &> &cb,
    Channel *channel, const FailFrame &ff) {
  std::string text;
  if (!cb(nextOutgoingId_, nextExpectedIncomingId_, &text, ff.nest(HERE))) {
    return false;
  }
  unacknowledgedOutgoing_.emplace_back(nextOutgoingId_, text);
  ++nextOutgoingId_;
  return channel->trySend(std::move(text), ff.nest(HERE));
}

bool Robustifier::noteIncoming(uint64_t incomingId, uint64_t nextExpectedOutgoingId) {
  if (incomingId != nextExpectedIncomingId_) {
    debug("NOTE: incoming %o != nextExp %o... ignoring", incomingId, nextExpectedIncomingId_);
    return false;
  }
  ++nextExpectedIncomingId_;

  while (!unacknowledgedOutgoing_.empty() &&
      unacknowledgedOutgoing_.front().first < nextExpectedOutgoingId) {
    unacknowledgedOutgoing_.pop_front();
  }

  return true;
}

bool Robustifier::tryCatchup(uint64_t nextExpectedOutgoingId, Channel *channel, const FailFrame &ff) {
  debug("Catching up to %o", nextExpectedOutgoingId);
  for (const auto &item : unacknowledgedOutgoing_) {
    if (item.first < nextExpectedOutgoingId) {
      continue;
    }
    if (!channel->trySend(item.second, ff.nest(HERE))) {
      return false;
    }
  }
  return true;
}
}  // namespace internal

FrontendRobustifier::FrontendRobustifier() : rb_(1000, 0) {}

bool FrontendRobustifier::trySendRequest(DRequest &&request, Channel *channel, const FailFrame &ff) {
  auto cb = [&request](uint64_t outId, uint64_t nextInId, std::string *result, const FailFrame &ff2) {
    using kosak::coding::tryAppendJson;
    CRequest creq(crequests::PackagedRequest(outId, nextInId, std::move(request)));
    return tryAppendJson(creq, result, ff2.nest(HERE));
  };
  return rb_.trySend(&cb, channel, ff.nest(HERE));
}

BackendRobustifier::BackendRobustifier() : rb_(0, 1000) {}

bool BackendRobustifier::trySendResponse(DResponse &&response, Channel *channel, const FailFrame &ff) {
  auto cb = [&response](uint64_t outId, uint64_t nextInId, std::string *result, const FailFrame &ff2) {
    using kosak::coding::tryAppendJson;
    CResponse cresp(cresponses::PackagedResponse(outId, nextInId, std::move(response)));
    return tryAppendJson(cresp, result, ff2.nest(HERE));
  };
  return rb_.trySend(&cb, channel, ff.nest(HERE));
}
}  // namespace z2kplus::backend::communicator
