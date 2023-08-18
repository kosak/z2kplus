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

#include "z2kplus/backend/test/util/fake_frontend.h"

#include "z2kplus/backend/communicator/message_buffer.h"
#include "z2kplus/backend/shared/protocol/control/crequest.h"
#include "z2kplus/backend/shared/protocol/control/cresponse.h"
#include "z2kplus/backend/shared/protocol/message/drequest.h"
#include "z2kplus/backend/shared/protocol/message/dresponse.h"
#include "z2kplus/backend/shared/profile.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::stringf;
using kosak::coding::ParseContext;
using z2kplus::backend::communicator::Channel;
using z2kplus::backend::communicator::ChannelMultiBuilder;
using z2kplus::backend::communicator::ChannelCallback;
using z2kplus::backend::communicator::FrontendRobustifier;
using z2kplus::backend::communicator::MessageBuffer;
using z2kplus::backend::shared::Profile;
using z2kplus::backend::shared::protocol::control::CRequest;
using z2kplus::backend::shared::protocol::control::CResponse;
using z2kplus::backend::shared::protocol::message::DRequest;
using z2kplus::backend::shared::protocol::message::DResponse;
using z2kplus::backend::shared::protocol::control::crequests::PackagedRequest;
using z2kplus::backend::shared::protocol::control::cresponses::PackagedResponse;

namespace crequests = z2kplus::backend::shared::protocol::control::crequests;
namespace cresponses = z2kplus::backend::shared::protocol::control::cresponses;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::test::util {
namespace internal {
class FrontendCallbacks final : public ChannelCallback {
public:
  explicit FrontendCallbacks(std::shared_ptr<FrontendRobustifier> rb);
  ~FrontendCallbacks() final;

  bool tryOnStartup(Channel *channel, const FailFrame &ff) final {
    return true;
  }
  bool tryOnMessage(Channel *channel, std::string &&message, const FailFrame &ff) final;
  bool tryOnShutdown(Channel *channel, const FailFrame &ff) final;

  void dropAllIncoming();

  std::shared_ptr<FrontendRobustifier> rb_;
  std::atomic<bool> dropAllIncoming_;
  MessageBuffer<CResponse> incomingControl_;
  MessageBuffer<DResponse> incomingData_;
};
}  // namespace internal

FakeFrontend::FakeFrontend() = default;
FakeFrontend::~FakeFrontend() = default;

bool FakeFrontend::tryCreate(std::string_view host, int port, std::string userId, std::string signature,
    std::optional<std::chrono::milliseconds> timeout, FakeFrontend *result, const FailFrame &ff) {
  auto rb = std::make_shared<FrontendRobustifier>();
  CRequest csReq((crequests::CreateSession()));
  return tryCreateHelper(host, port, std::move(userId), std::move(signature), timeout,
    std::move(rb), csReq, result, ff.nest(HERE));
}

bool FakeFrontend::tryAttach(std::string_view host, int port, std::string userId, std::string signature,
    std::optional<std::chrono::milliseconds> timeout, std::string existingSessionId,
    std::shared_ptr<FrontendRobustifier> rb, FakeFrontend *result, const FailFrame &ff) {
  CRequest csReq(crequests::AttachToSession(std::move(existingSessionId), rb->nextExpectedResponseId()));
  return tryCreateHelper(host, port, std::move(userId), std::move(signature), timeout,
      std::move(rb), csReq, result, ff.nest(HERE));
}

bool FakeFrontend::tryCreateHelper(std::string_view host, int port, std::string userId,
    std::string signature, std::optional<std::chrono::milliseconds> timeout,
    std::shared_ptr<FrontendRobustifier> rb, const CRequest &request, FakeFrontend *result,
    const FailFrame &ff) {
  auto myCallbacks = std::make_shared<internal::FrontendCallbacks>(rb);
  std::shared_ptr<Channel> channel;

  Profile profile(std::move(userId), std::move(signature));
  CRequest helloReq(crequests::Hello(std::move(profile)));
  ChannelMultiBuilder mb;
  using kosak::coding::tryAppendJson;
  auto *next = mb.startNextCommand();
  if (!tryAppendJson(helloReq, next, ff.nest(HERE))) {
    return false;
  }
  next = mb.startNextCommand();
  if (!tryAppendJson(request, next, ff.nest(HERE))) {
    return false;
  }

  MySocket socket;
  if (!MySocket::tryConnect(host, port, &socket, ff.nest(HERE)) ||
      !Channel::tryCreate("FFE", std::move(socket), myCallbacks, &channel, ff.nest(HERE)) ||
      !channel->trySend(mb.releaseBuffer(), ff.nest(HERE))) {
    return false;
  }

  std::vector<CResponse> responses;
  bool wantShutdown;
  myCallbacks->incomingControl_.waitForDataAndSwap(timeout, &responses, &wantShutdown);
  if (responses.empty()) {
    return ff.fail(HERE, "Timeout waiting for sessionId");
  }

  const auto *ss = std::get_if<cresponses::SessionSuccess>(&responses.front().payload());
  if (ss == nullptr) {
    return ff.fail(HERE, "Create session failed apparently.");
  }

  if (!rb->tryCatchup(ss->nextExpectedRequestId(), channel.get(), ff.nest(HERE))) {
    return false;
  }
  *result = FakeFrontend(ss->assignedSessionGuid(), std::move(channel), std::move(rb),
      std::move(myCallbacks));
  return true;
}

FakeFrontend::FakeFrontend(std::string sessionId, std::shared_ptr<Channel> &&channel,
    std::shared_ptr<FrontendRobustifier> &&rb, std::shared_ptr<internal::FrontendCallbacks> &&callbacks) :
    sessionId_(std::move(sessionId)), channel_(std::move(channel)), rb_(std::move(rb)),
    callbacks_(std::move(callbacks)) {}

//bool FakeFrontend::tryAttachToSession(MySocket socket, size_t timeoutMillis, Failures *failures) {
//  // new callbacks sharing same robustifier
//  auto newCallbacks = std::make_shared<internal::FrontendCallbacks>(callbacks_->rb_);
//  CRequest creq(crequests::AttachToSession(nextRequestId_++, userId_, sessionId_,
//      rb_->nextExpectedResponseId()));
//  auto cb = [&creq](std::string *result, bool *again, Failures *f2) {
//    using kosak::coding::tryAppendJson;
//    return tryAppendJson(creq, result, f2);
//  };
//  std::shared_ptr<Channel> newChannel;
//  if (!Channel::tryCreate("FFE", std::move(socket), &newChannel, failures) ||
//      !newChannel->tryStart(newCallbacks, failures) ||
//      !newChannel->trySend(&cb, failures)) {
//    return false;
//  }
//
//  std::vector<CResponse> result;
//  bool wantShutdown;
//  newCallbacks->incomingControl_.waitForDataAndSwap(timeoutMillis, &result, &wantShutdown);
//  if (result.empty()) {
//    failures->add("Timeout waiting for sessionId");
//    return false;
//  }
//
//  const auto *ss = std::get_if<cresponses::SessionSuccess>(&result.front().payload());
//  if (ss == nullptr) {
//    failures->add("Attach to session failed apparently.");
//    return false;
//  }
//  callbacks_ = std::move(newCallbacks);
//  channel_ = std::move(newChannel);
//  return rb_->tryCatchup(ss->nextExpectedRequestId(), channel_.get(), failures);
//}

bool FakeFrontend::trySend(DRequest &&request, const FailFrame &ff) {
  return rb_->trySendRequest(std::move(request), channel_.get(), ff.nest(HERE));
}

void FakeFrontend::startDroppingIncoming() {
  callbacks_->dropAllIncoming();
}

void FakeFrontend::waitForDataAndSwap(std::optional<std::chrono::milliseconds> timeout,
    std::vector<DResponse> *responseBuffer, bool *wantShutdown) {
  callbacks_->incomingData_.waitForDataAndSwap(timeout, responseBuffer, wantShutdown);
}

namespace internal {
FrontendCallbacks::FrontendCallbacks(std::shared_ptr<FrontendRobustifier> rb) : rb_(std::move(rb)),
  dropAllIncoming_(false) {}
FrontendCallbacks::~FrontendCallbacks() = default;

bool FrontendCallbacks::tryOnMessage(Channel *channel, std::string &&message, const FailFrame &ff) {
  if (dropAllIncoming_) {
    debug("DROPPED THIS MESSAGE: %o", message);
    return true;
  }
  CResponse response;
  ParseContext ctx(message);
  using kosak::coding::tryParseJson;
  if (!tryParseJson(&ctx, &response, ff.nest(HERE))) {
    return false;
  }
  auto *pr = std::get_if<cresponses::PackagedResponse>(&response.payload());
  if (pr == nullptr) {
    incomingControl_.append(std::move(response));
    return true;
  }

  if (!rb_->noteIncomingResponse(*pr)) {
    // Response is duplicate. Not an error.
    return true;
  }

  incomingData_.append(std::move(pr->response()));
  return true;
}

bool FrontendCallbacks::tryOnShutdown(Channel *channel, const FailFrame &ff) {
  incomingControl_.shutdown();
  incomingData_.shutdown();
  return true;
}

void FrontendCallbacks::dropAllIncoming() {
  dropAllIncoming_ = true;
}
}  // namespace internal
}  // namespace z2kplus::backend::test::util
