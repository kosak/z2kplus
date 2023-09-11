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
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/myjson.h"
#include "z2kplus/backend/shared/profile.h"
#include "z2kplus/backend/shared/protocol/misc.h"
#include "z2kplus/backend/shared/zephyrgram.h"

namespace z2kplus::backend::shared::protocol::message {

namespace dresponses {

class AckSyntaxCheck {
public:
  AckSyntaxCheck();
  AckSyntaxCheck(std::string text, bool valid, std::string result);
  DISALLOW_COPY_AND_ASSIGN(AckSyntaxCheck);
  DECLARE_MOVE_COPY_AND_ASSIGN(AckSyntaxCheck);
  ~AckSyntaxCheck();

  const std::string &text() const { return text_; }
  bool valid() const { return valid_; }
  const std::string &result() const { return result_; }

private:
  std::string text_;
  bool valid_ = false;
  std::string result_;

  friend std::ostream &operator<<(std::ostream &s, const AckSyntaxCheck &o);
  DECLARE_TYPICAL_JSON(AckSyntaxCheck);
};

class AckSubscribe {
public:
  AckSubscribe();
  AckSubscribe(bool valid, std::string humanReadableError, Estimates estimates);
  DISALLOW_COPY_AND_ASSIGN(AckSubscribe);
  DECLARE_MOVE_COPY_AND_ASSIGN(AckSubscribe);
  ~AckSubscribe();

  bool valid() const { return valid_; }
  const std::string &humanReadableError() const { return humanReadableError_; }
  const Estimates &estimates() const { return estimates_; }

private:
  bool valid_ = false;
  std::string humanReadableError_;
  Estimates estimates_;

  friend std::ostream &operator<<(std::ostream &s, const AckSubscribe &o);
  DECLARE_TYPICAL_JSON(AckSubscribe);
};

// Here's the thing. These "shared" data structures, which are normally just a place to
// temporarily hold values to and from their JSON representation, would *not* normally use
// shared_ptr in their representation. However, in this case, this plays so nicely with our cache,
// that I'm going to do it anyway. In the future I might have these objects be some kind of
// interface representation or something.
class AckMoreZgrams {
public:
  AckMoreZgrams();
  AckMoreZgrams(bool forBackside, std::vector<std::shared_ptr<const Zephyrgram>> zgrams,
      Estimates estimates);
  DISALLOW_COPY_AND_ASSIGN(AckMoreZgrams);
  DECLARE_MOVE_COPY_AND_ASSIGN(AckMoreZgrams);
  ~AckMoreZgrams();

  std::vector<std::shared_ptr<const Zephyrgram>> &zgrams() { return zgrams_; }
  const std::vector<std::shared_ptr<const Zephyrgram>> &zgrams() const { return zgrams_; }

  bool forBackside() const { return forBackside_; }
  Estimates &estimates() { return estimates_; }
  const Estimates &estimates() const { return estimates_; }

private:
  bool forBackside_ = false;
  std::vector<std::shared_ptr<const Zephyrgram>> zgrams_;
  Estimates estimates_;

  friend std::ostream &operator<<(std::ostream &s, const AckMoreZgrams &o);
  DECLARE_TYPICAL_JSON(AckMoreZgrams);
};

class EstimatesUpdate {
public:
  EstimatesUpdate();
  explicit EstimatesUpdate(Estimates estimates);
  DISALLOW_COPY_AND_ASSIGN(EstimatesUpdate);
  DECLARE_MOVE_COPY_AND_ASSIGN(EstimatesUpdate);
  ~EstimatesUpdate();

  const Estimates &estimates() const { return estimates_; }

private:
  Estimates estimates_;

  friend std::ostream &operator<<(std::ostream &s, const EstimatesUpdate &o);
  DECLARE_TYPICAL_JSON(EstimatesUpdate);
};

class MetadataUpdate {
public:
  MetadataUpdate();
  explicit MetadataUpdate(std::vector<std::shared_ptr<const MetadataRecord>> metadata);
  DISALLOW_COPY_AND_ASSIGN(MetadataUpdate);
  DECLARE_MOVE_COPY_AND_ASSIGN(MetadataUpdate);
  ~MetadataUpdate();

  std::vector<std::shared_ptr<const MetadataRecord>> &metadata() { return metadata_; }
  const std::vector<std::shared_ptr<const MetadataRecord>> &metadata() const { return metadata_; }

private:
  std::vector<std::shared_ptr<const MetadataRecord>> metadata_;

  friend std::ostream &operator<<(std::ostream &s, const MetadataUpdate &o);
  DECLARE_TYPICAL_JSON(MetadataUpdate);
};

class AckSpecificZgrams {
public:
  AckSpecificZgrams();
  explicit AckSpecificZgrams(std::vector<std::shared_ptr<const Zephyrgram>> zgrams);
  DISALLOW_COPY_AND_ASSIGN(AckSpecificZgrams);
  DECLARE_MOVE_COPY_AND_ASSIGN(AckSpecificZgrams);
  ~AckSpecificZgrams();

  std::vector<std::shared_ptr<const Zephyrgram>> &zgrams() { return zgrams_; }
  const std::vector<std::shared_ptr<const Zephyrgram>> &zgrams() const { return zgrams_; }

private:
  std::vector<std::shared_ptr<const Zephyrgram>> zgrams_;

  friend std::ostream &operator<<(std::ostream &s, const AckSpecificZgrams &o);
  DECLARE_TYPICAL_JSON(AckSpecificZgrams);
};

class PlusPlusUpdate {
public:
  typedef std::tuple<ZgramId, std::string, int64_t> entry_t;

  PlusPlusUpdate();
  explicit PlusPlusUpdate(std::vector<entry_t> updates);
  DISALLOW_COPY_AND_ASSIGN(PlusPlusUpdate);
  DECLARE_MOVE_COPY_AND_ASSIGN(PlusPlusUpdate);
  ~PlusPlusUpdate();

  std::vector<entry_t> &updates() { return updates_; }
  const std::vector<entry_t> &updates() const { return updates_; }

private:
  std::vector<entry_t> updates_;

  friend std::ostream &operator<<(std::ostream &s, const PlusPlusUpdate &o);
  DECLARE_TYPICAL_JSON(PlusPlusUpdate);
};

class AckPing {
  typedef kosak::coding::FailFrame FailFrame;
  typedef kosak::coding::ParseContext ParseContext;
public:
  AckPing() = default;
  explicit AckPing(uint64_t cookie) : cookie_(cookie) {}
  DISALLOW_COPY_AND_ASSIGN(AckPing);
  DEFINE_MOVE_COPY_AND_ASSIGN(AckPing);
  ~AckPing() = default;

  uint64_t cookie() const { return cookie_; }

private:
  uint64_t cookie_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const AckPing &o);
  DECLARE_TYPICAL_JSON(AckPing);
};

class GeneralError {
public:
  GeneralError() = default;
  explicit GeneralError(std::string message) : message_(std::move(message)) {}
  DISALLOW_COPY_AND_ASSIGN(GeneralError);
  DEFINE_MOVE_COPY_AND_ASSIGN(GeneralError);
  ~GeneralError() = default;

private:
  std::string message_;

  friend std::ostream &operator<<(std::ostream &s, const GeneralError &o);
  DECLARE_TYPICAL_JSON(GeneralError);
};

typedef std::variant<AckSyntaxCheck, AckSubscribe, AckMoreZgrams,
    EstimatesUpdate, MetadataUpdate, AckSpecificZgrams, PlusPlusUpdate,
    AckPing, GeneralError> payload_t;

DECLARE_VARIANT_JSON(PayloadHolder, payload_t,
    ("AckSyntaxCheck", "AckSubscribe", "AckMoreZgrams",
        "EstimatesUpdate", "MetadataUpdate", "AckSpecificZgrams", "PlusPlusUpdate",
        "AckPing", "GeneralError"));
}  // namespace dresponses

class DResponse {
public:
  DResponse();
  explicit DResponse(dresponses::AckSyntaxCheck o);
  explicit DResponse(dresponses::AckSubscribe o);
  explicit DResponse(dresponses::AckMoreZgrams o);
  explicit DResponse(dresponses::EstimatesUpdate o);
  explicit DResponse(dresponses::MetadataUpdate o);
  explicit DResponse(dresponses::AckSpecificZgrams o);
  explicit DResponse(dresponses::PlusPlusUpdate o);
  explicit DResponse(dresponses::AckPing o);
  explicit DResponse(dresponses::GeneralError o);
  DISALLOW_COPY_AND_ASSIGN(DResponse);
  DECLARE_MOVE_COPY_AND_ASSIGN(DResponse);
  ~DResponse();

  dresponses::payload_t &payload() { return payload_; }
  const dresponses::payload_t &payload() const { return payload_; }

private:
  dresponses::payload_t payload_;

  friend std::ostream &operator<<(std::ostream &s, const DResponse &o);
  DECLARE_TYPICAL_JSON(DResponse);
};

}  // namespace z2kplus::backend::shared::protocol::message
