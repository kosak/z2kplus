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

#include "z2kplus/backend/shared/protocol/control/crequest.h"

#include <string>
#include "kosak/coding/failures.h"
#include "kosak/coding/myjson.h"

using kosak::coding::ParseContext;
using kosak::coding::streamf;

namespace z2kplus::backend::shared::protocol::control {
namespace crequests {
Hello::Hello() = default;
Hello::Hello(Profile profile) : profile_(std::move(profile)) {}
Hello::Hello(Hello &&) noexcept = default;
Hello &Hello::operator=(Hello &&) noexcept = default;
Hello::~Hello() = default;

std::ostream &operator<<(std::ostream &s, const Hello &o) {
  return streamf(s, "Hello(%o)", o.profile_);
}

DEFINE_TYPICAL_JSON(Hello, profile_);

CreateSession::CreateSession() = default;
CreateSession::CreateSession(CreateSession &&) noexcept = default;
CreateSession &CreateSession::operator=(CreateSession &&) noexcept = default;
CreateSession::~CreateSession() = default;

std::ostream &operator<<(std::ostream &s, const CreateSession &o) {
  return s << "CreateSession()";
}
DEFINE_TYPICAL_JSON(CreateSession);

AttachToSession::AttachToSession() = default;
AttachToSession::AttachToSession(std::string existingSessionGuid, uint64 nextExpectedResponseId) :
    existingSessionGuid_(std::move(existingSessionGuid)), nextExpectedResponseId_(nextExpectedResponseId) {}
AttachToSession::AttachToSession(AttachToSession &&) noexcept = default;
AttachToSession &AttachToSession::operator=(AttachToSession &&) noexcept = default;
AttachToSession::~AttachToSession() = default;

std::ostream &operator<<(std::ostream &s, const AttachToSession &o) {
  return streamf(s, "AttachToSession(%o, %o)", o.existingSessionGuid_, o.nextExpectedResponseId_);
}
DEFINE_TYPICAL_JSON(AttachToSession, existingSessionGuid_, nextExpectedResponseId_);

PackagedRequest::PackagedRequest() = default;
PackagedRequest::PackagedRequest(uint64_t requestId, uint64_t nextExpectedResponseId,
    DRequest &&request) : requestId_(requestId), nextExpectedResponseId_(nextExpectedResponseId),
    request_(std::move(request)) {}
PackagedRequest::PackagedRequest(PackagedRequest &&) noexcept = default;
PackagedRequest &PackagedRequest::operator=(PackagedRequest &&) noexcept = default;
PackagedRequest::~PackagedRequest() = default;

std::ostream &operator<<(std::ostream &s, const PackagedRequest &o) {
  return streamf(s, "PackagedRequest(%o, %o, %o)", o.requestId_, o.nextExpectedResponseId_,
      o.request_);
}
DEFINE_TYPICAL_JSON(PackagedRequest, requestId_, nextExpectedResponseId_, request_);
}  // namespace crequests

CRequest::CRequest() = default;
CRequest::CRequest(crequests::Hello &&o) : payload_(std::move(o)) {}
CRequest::CRequest(crequests::CreateSession &&o) : payload_(std::move(o)) {}
CRequest::CRequest(crequests::AttachToSession &&o) : payload_(std::move(o)) {}
CRequest::CRequest(crequests::PackagedRequest &&o) : payload_(std::move(o)) {}
CRequest::CRequest(CRequest &&) noexcept = default;
CRequest &CRequest::operator=(CRequest &&) noexcept = default;
CRequest::~CRequest() = default;

DEFINE_TYPICAL_JSON(CRequest, payload_);
}  // namespace z2kplus::backend::shared::protocol::control
