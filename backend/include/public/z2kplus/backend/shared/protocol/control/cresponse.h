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

#include <iostream>
#include "kosak/coding/coding.h"
#include "kosak/coding/myjson.h"
#include "z2kplus/backend/shared/protocol/misc.h"
#include "z2kplus/backend/shared/protocol/message/dresponse.h"

namespace z2kplus::backend::shared::protocol::control {

namespace cresponses {
class SessionSuccess {
  typedef kosak::coding::ParseContext ParseContext;

public:
  SessionSuccess() = default;
  SessionSuccess(std::string assignedSessionGuid, uint64_t nextExpectedRequestId, Profile profile);
  DISALLOW_COPY_AND_ASSIGN(SessionSuccess);
  DEFINE_MOVE_COPY_AND_ASSIGN(SessionSuccess);
  ~SessionSuccess() = default;

  const std::string &assignedSessionGuid() const { return assignedSessionGuid_; }
  uint64_t nextExpectedRequestId() const { return nextExpectedRequestId_; }
  const Profile &profile() const { return profile_; }
  Profile &profile() { return profile_; }

private:
  std::string assignedSessionGuid_;
  uint64_t nextExpectedRequestId_ = 0;
  Profile profile_;

  friend std::ostream &operator<<(std::ostream &s, const SessionSuccess &o);
  DECLARE_TYPICAL_JSON(SessionSuccess);
};

class SessionFailure {
  typedef kosak::coding::ParseContext ParseContext;

public:
  SessionFailure() = default;
  DISALLOW_COPY_AND_ASSIGN(SessionFailure);
  DEFINE_MOVE_COPY_AND_ASSIGN(SessionFailure);
  ~SessionFailure() = default;

private:
  friend std::ostream &operator<<(std::ostream &s, const SessionFailure &o);
  DECLARE_TYPICAL_JSON(SessionFailure);
};

class PackagedResponse {
  typedef z2kplus::backend::shared::protocol::message::DResponse DResponse;

public:
  PackagedResponse();
  PackagedResponse(uint64_t responseId, uint64_t nextExpectedRequestId, DResponse &&dResponse);
  DISALLOW_COPY_AND_ASSIGN(PackagedResponse);
  DECLARE_MOVE_COPY_AND_ASSIGN(PackagedResponse);
  ~PackagedResponse();

  uint64_t responseId() const { return responseId_; }
  uint64_t nextExpectedRequestId() const { return nextExpectedRequestId_; }
  DResponse &response() { return response_; }
  const DResponse &response() const { return response_; }

private:
  uint64_t responseId_ = 0;
  uint64_t nextExpectedRequestId_ = 0;
  DResponse response_;

  friend std::ostream &operator<<(std::ostream &s, const PackagedResponse &o);
  DECLARE_TYPICAL_JSON(PackagedResponse);
};

typedef std::variant<SessionSuccess, SessionFailure, PackagedResponse> payload_t;
DECLARE_VARIANT_JSON(PayloadHolder, payload_t, ("SessionSuccess", "SessionFailure", "PackagedResponse"));
}  // namespace cresponses

class CResponse {
  typedef kosak::coding::ParseContext ParseContext;

public:
  CResponse();
  explicit CResponse(cresponses::SessionSuccess &&o);
  explicit CResponse(cresponses::SessionFailure &&o);
  explicit CResponse(cresponses::PackagedResponse &&o);
  DISALLOW_COPY_AND_ASSIGN(CResponse);
  DECLARE_MOVE_COPY_AND_ASSIGN(CResponse);
  ~CResponse();

  cresponses::payload_t &payload() { return payload_; }
  const cresponses::payload_t &payload() const { return payload_; }

private:
  cresponses::payload_t payload_;

  friend std::ostream &operator<<(std::ostream &s, const CResponse &o) {
    return s << o.payload_;
  }

  DECLARE_TYPICAL_JSON(CResponse);
};
}  // namespace z2kplus::backend::shared::protocol::control
