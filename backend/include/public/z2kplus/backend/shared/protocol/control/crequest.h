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

#include <cstdint>
#include <iostream>
#include <string>
#include "kosak/coding/coding.h"
#include "kosak/coding/myjson.h"
#include "z2kplus/backend/shared/profile.h"
#include "z2kplus/backend/shared/protocol/misc.h"
#include "z2kplus/backend/shared/protocol/message/drequest.h"

namespace z2kplus::backend::shared::protocol::control {
namespace crequests {
class Hello {
public:
  Hello();
  explicit Hello(Profile profile);
  DISALLOW_COPY_AND_ASSIGN(Hello);
  DECLARE_MOVE_COPY_AND_ASSIGN(Hello);
  ~Hello();

  Profile &profile() { return profile_; }
  const Profile &profile() const { return profile_; }

private:
  Profile profile_;

  friend std::ostream &operator<<(std::ostream &s, const Hello &o);
  DECLARE_TYPICAL_JSON(Hello);
};

class CreateSession {
public:
  CreateSession();
  DISALLOW_COPY_AND_ASSIGN(CreateSession);
  DECLARE_MOVE_COPY_AND_ASSIGN(CreateSession);
  ~CreateSession();

private:
  friend std::ostream &operator<<(std::ostream &s, const CreateSession &o);
  DECLARE_TYPICAL_JSON(CreateSession);
};

class AttachToSession {
public:
  AttachToSession();
  AttachToSession(std::string existingSessionGuid, uint64_t nextExpectedResponseId);
  DISALLOW_COPY_AND_ASSIGN(AttachToSession);
  DECLARE_MOVE_COPY_AND_ASSIGN(AttachToSession);
  ~AttachToSession();

  const std::string &existingSessionGuid() const { return existingSessionGuid_; }
  uint64_t nextExpectedResponseId() const { return nextExpectedResponseId_; }

private:
  std::string existingSessionGuid_;
  uint64_t nextExpectedResponseId_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const AttachToSession &o);
  DECLARE_TYPICAL_JSON(AttachToSession);
};

class PackagedRequest {
  typedef z2kplus::backend::shared::protocol::message::DRequest DRequest;

public:
  PackagedRequest();
  PackagedRequest(uint64_t requestId, uint64_t nextExpectedResponseId, DRequest &&request);
  DISALLOW_COPY_AND_ASSIGN(PackagedRequest);
  DECLARE_MOVE_COPY_AND_ASSIGN(PackagedRequest);
  ~PackagedRequest();

  uint64_t requestId() const { return requestId_; }
  uint64_t nextExpectedResponseId() const { return nextExpectedResponseId_; }
  const DRequest &request() const { return request_; }
  DRequest &request() { return request_; }

private:
  uint64_t requestId_ = 0;
  uint64_t nextExpectedResponseId_ = 0;
  DRequest request_;

  friend std::ostream &operator<<(std::ostream &s, const PackagedRequest &o);
  DECLARE_TYPICAL_JSON(PackagedRequest);
};

typedef std::variant<Hello, CreateSession, AttachToSession, PackagedRequest> payload_t;
DECLARE_VARIANT_JSON(PayloadHolder, payload_t, ("Hello", "CreateSession", "AttachToSession", "PackagedRequest"));
}  // namespace crequests

class CRequest {
public:
  CRequest();
  explicit CRequest(crequests::Hello &&o);
  explicit CRequest(crequests::CreateSession &&o);
  explicit CRequest(crequests::AttachToSession &&o);
  explicit CRequest(crequests::PackagedRequest &&o);
  DISALLOW_COPY_AND_ASSIGN(CRequest);
  DECLARE_MOVE_COPY_AND_ASSIGN(CRequest);
  ~CRequest();

  crequests::payload_t &payload() { return payload_; }
  const crequests::payload_t &payload() const { return payload_; }

private:
  crequests::payload_t payload_;

  friend std::ostream &operator<<(std::ostream &s, const CRequest &o) {
    return s << o.payload_;
  }
  DECLARE_TYPICAL_JSON(CRequest);
};
}  // namespace z2kplus::backend::shared::protocol::control
