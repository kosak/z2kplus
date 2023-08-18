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
#include <string>
#include <vector>
#include "kosak/coding/failures.h"
#include "z2kplus/backend/communicator/robustifier.h"
#include "z2kplus/backend/server/server.h"
#include "z2kplus/backend/shared/protocol/control/crequest.h"
#include "z2kplus/backend/shared/protocol/message/drequest.h"
#include "z2kplus/backend/shared/protocol/message/dresponse.h"
#include "z2kplus/backend/util/mysocket.h"

namespace z2kplus::backend::test::util {
namespace internal {
class FrontendCallbacks;
}  // namespace internal

class FakeFrontend final {
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::communicator::Channel Channel;
  typedef z2kplus::backend::communicator::FrontendRobustifier FrontendRobustifier;
  typedef z2kplus::backend::shared::protocol::control::CRequest CRequest;
  typedef z2kplus::backend::shared::protocol::message::DRequest DRequest;
  typedef z2kplus::backend::shared::protocol::message::DResponse DResponse;
  typedef z2kplus::backend::shared::protocol::control::crequests::PackagedRequest PackagedRequest;
  typedef z2kplus::backend::shared::protocol::control::cresponses::PackagedResponse PackagedResponse;
  typedef z2kplus::backend::util::MySocket MySocket;

public:
  static bool tryCreate(std::string_view host, int port, std::string userId, std::string signature,
      std::optional<std::chrono::milliseconds> timeout, FakeFrontend *result, const FailFrame &ff);
  static bool tryAttach(std::string_view host, int port, std::string userId, std::string signature,
      std::optional<std::chrono::milliseconds> timeout, std::string existingSessionId,
      std::shared_ptr<FrontendRobustifier> rb, FakeFrontend *result, const FailFrame &ff);

  FakeFrontend();
  ~FakeFrontend();

  bool trySend(DRequest &&request, const FailFrame &ff);

  void startDroppingIncoming();

  void waitForDataAndSwap(std::optional<std::chrono::milliseconds>,
      std::vector<DResponse> *responseBuffer, bool *wantShutdown);

  const std::string &sessionId() const { return sessionId_; }
  const std::shared_ptr<FrontendRobustifier> &robustifier() const { return rb_; }

private:
  static bool tryCreateHelper(std::string_view host, int port, std::string userId,
      std::string signature, std::optional<std::chrono::milliseconds> timeout,
      std::shared_ptr<FrontendRobustifier> rb, const CRequest &request,
      FakeFrontend *result, const FailFrame &ff);

  FakeFrontend(std::string sessionId, std::shared_ptr<Channel> &&channel,
      std::shared_ptr<FrontendRobustifier> &&rb, std::shared_ptr<internal::FrontendCallbacks> &&callbacks);

  std::string sessionId_;
  std::shared_ptr<Channel> channel_;
  std::shared_ptr<FrontendRobustifier> rb_;
  std::shared_ptr<internal::FrontendCallbacks> callbacks_;
};
}  // namespace z2kplus::backend::test::util
