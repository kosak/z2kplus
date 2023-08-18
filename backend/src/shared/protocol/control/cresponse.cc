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

#include "z2kplus/backend/shared/protocol/control/cresponse.h"

#include <string>
#include "kosak/coding/failures.h"
#include "kosak/coding/myjson.h"

using kosak::coding::ParseContext;
using kosak::coding::streamf;

namespace z2kplus::backend::shared::protocol::control {
namespace cresponses {

DEFINE_VARIANT_JSON(PayloadHolder, payload_t);

SessionSuccess::SessionSuccess(std::string assignedSessionGuid, uint64_t nextExpectedRequestId,
    Profile profile) : assignedSessionGuid_(std::move(assignedSessionGuid)),
    nextExpectedRequestId_(nextExpectedRequestId), profile_(std::move(profile)) {
}

std::ostream &operator<<(std::ostream &s, const SessionSuccess &o) {
  return streamf(s, "SessionSuccess(%o, %o, %o)", o.assignedSessionGuid_, o.nextExpectedRequestId_,
      o.profile_);
}

DEFINE_TYPICAL_JSON(SessionSuccess, assignedSessionGuid_, nextExpectedRequestId_, profile_);

std::ostream &operator<<(std::ostream &s, const SessionFailure &o) {
  return s << "SessionFailure()";
}

DEFINE_TYPICAL_JSON(SessionFailure);

PackagedResponse::PackagedResponse() = default;
PackagedResponse::PackagedResponse(uint64_t responseId, uint64_t nextExpectedRequestId,
    DResponse &&response) : responseId_(responseId), nextExpectedRequestId_(nextExpectedRequestId),
    response_(std::move(response)) {}
PackagedResponse::PackagedResponse(PackagedResponse &&) noexcept = default;
PackagedResponse &PackagedResponse::operator=(PackagedResponse &&) noexcept = default;
PackagedResponse::~PackagedResponse() = default;

std::ostream &operator<<(std::ostream &s, const PackagedResponse &o) {
  return streamf(s, "PackagedResponse(%o, %o, %o)", o.responseId_, o.nextExpectedRequestId_,
      o.response_);
}
DEFINE_TYPICAL_JSON(PackagedResponse, responseId_, nextExpectedRequestId_, response_);
}  // namespace cresponses

CResponse::CResponse() = default;
CResponse::CResponse(cresponses::SessionSuccess &&o) : payload_(std::move(o)) {}
CResponse::CResponse(cresponses::SessionFailure &&o) : payload_(std::move(o)) {}
CResponse::CResponse(cresponses::PackagedResponse &&o) : payload_(std::move(o)) {}
CResponse::CResponse(CResponse &&) noexcept = default;
CResponse &CResponse::operator=(CResponse &&) noexcept = default;
CResponse::~CResponse() = default;

DEFINE_TYPICAL_JSON(CResponse, payload_);
}  // namespace z2kplus::backend::shared::protocol::control
