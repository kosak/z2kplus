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

#include "z2kplus/backend/shared/protocol/message/dresponse.h"
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/myjson.h"
#include "z2kplus/backend/shared/zephyrgram.h"

using kosak::coding::ParseContext;
using kosak::coding::streamf;

namespace z2kplus::backend::shared::protocol::message {
namespace dresponses {
AckSyntaxCheck::AckSyntaxCheck() = default;
AckSyntaxCheck::AckSyntaxCheck(std::string text, bool valid, std::string result)
  : text_(std::move(text)), valid_(valid), result_(std::move(result)) {}
AckSyntaxCheck::AckSyntaxCheck(AckSyntaxCheck &&other) noexcept = default;
AckSyntaxCheck &AckSyntaxCheck::operator=(AckSyntaxCheck &&other) noexcept = default;
AckSyntaxCheck::~AckSyntaxCheck() = default;

std::ostream &operator<<(std::ostream &s, const AckSyntaxCheck &o) {
  return streamf(s, "AckSyntaxCheck(%o,%o,%o)", o.text_, o.valid_, o.result_);
}

DEFINE_TYPICAL_JSON(AckSyntaxCheck, text_, valid_, result_);

AckSubscribe::AckSubscribe() = default;
AckSubscribe::AckSubscribe(bool valid, std::string humanReadableError, Estimates estimates) :
    valid_(valid), humanReadableError_(std::move(humanReadableError)),
    estimates_(std::move(estimates)) {}
AckSubscribe::AckSubscribe(AckSubscribe &&other) noexcept = default;
AckSubscribe &AckSubscribe::operator=(AckSubscribe &&other) noexcept = default;
AckSubscribe::~AckSubscribe() = default;

std::ostream &operator<<(std::ostream &s, const AckSubscribe &o) {
  return streamf(s, "AckSubscribe(%o,%o,%o)", o.valid_, o.humanReadableError_, o.estimates_);
}

DEFINE_TYPICAL_JSON(AckSubscribe, valid_, humanReadableError_, estimates_);

AckMoreZgrams::AckMoreZgrams() = default;
AckMoreZgrams::AckMoreZgrams(bool forBackside, std::vector<std::shared_ptr<const Zephyrgram>> zgrams,
    Estimates estimates) :
    forBackside_(forBackside), zgrams_(std::move(zgrams)), estimates_(std::move(estimates)) {}
AckMoreZgrams::AckMoreZgrams(AckMoreZgrams &&other) noexcept = default;
AckMoreZgrams &AckMoreZgrams::operator=(AckMoreZgrams &&other) noexcept = default;
AckMoreZgrams::~AckMoreZgrams() = default;

std::ostream &operator<<(std::ostream &s, const AckMoreZgrams &o) {
  return streamf(s, "AckMoreZgrams(%o,%o,%o)", o.forBackside_, o.zgrams_, o.estimates_);
}
DEFINE_TYPICAL_JSON(AckMoreZgrams, forBackside_, zgrams_, estimates_);

EstimatesUpdate::EstimatesUpdate() = default;
EstimatesUpdate::EstimatesUpdate(Estimates estimates) : estimates_(std::move(estimates)) {}
EstimatesUpdate::EstimatesUpdate(EstimatesUpdate &&other) noexcept = default;
EstimatesUpdate &EstimatesUpdate::operator=(EstimatesUpdate &&other) noexcept = default;
EstimatesUpdate::~EstimatesUpdate() = default;

std::ostream &operator<<(std::ostream &s, const EstimatesUpdate &o) {
  return streamf(s, "EstimatesUpdate(%o)", o.estimates_);
}

DEFINE_TYPICAL_JSON(EstimatesUpdate, estimates_);

MetadataUpdate::MetadataUpdate() = default;
MetadataUpdate::MetadataUpdate(std::vector<std::shared_ptr<const MetadataRecord>> metadata) :
    metadata_(std::move(metadata)) {}
MetadataUpdate::MetadataUpdate(MetadataUpdate &&other) noexcept = default;
MetadataUpdate &MetadataUpdate::operator=(MetadataUpdate &&other) noexcept = default;
MetadataUpdate::~MetadataUpdate() = default;

std::ostream &operator<<(std::ostream &s, const MetadataUpdate &o) {
  return streamf(s, "MetadataUpdate(%o)", o.metadata_);
}
DEFINE_TYPICAL_JSON(MetadataUpdate, metadata_);

AckSpecificZgrams::AckSpecificZgrams() = default;
AckSpecificZgrams::AckSpecificZgrams(std::vector<std::shared_ptr<const Zephyrgram>> zgrams) :
    zgrams_(std::move(zgrams)) {}
AckSpecificZgrams::AckSpecificZgrams(AckSpecificZgrams &&other) noexcept = default;
AckSpecificZgrams &AckSpecificZgrams::operator=(AckSpecificZgrams &&other) noexcept = default;
AckSpecificZgrams::~AckSpecificZgrams() = default;

std::ostream &operator<<(std::ostream &s, const AckSpecificZgrams &o) {
  return streamf(s, "AckSpecificZgrams(%o)", o.zgrams_);
}
DEFINE_TYPICAL_JSON(AckSpecificZgrams, zgrams_);

PlusPlusUpdate::PlusPlusUpdate() = default;
PlusPlusUpdate::PlusPlusUpdate(std::vector<entry_t> updates) : updates_(std::move(updates)) {}
PlusPlusUpdate::PlusPlusUpdate(PlusPlusUpdate &&other) noexcept = default;
PlusPlusUpdate &PlusPlusUpdate::operator=(PlusPlusUpdate &&other) noexcept = default;
PlusPlusUpdate::~PlusPlusUpdate() = default;

std::ostream &operator<<(std::ostream &s, const PlusPlusUpdate &o) {
  return streamf(s, "PlusPlusUpdate(%o)", o.updates_);
}
DEFINE_TYPICAL_JSON(PlusPlusUpdate, updates_);

FiltersUpdate::FiltersUpdate() = default;
FiltersUpdate::FiltersUpdate(uint64_t version, std::vector<Filter> filters) :
  version_(version), filters_(std::move(filters)) {}
FiltersUpdate::FiltersUpdate(FiltersUpdate &&other) noexcept = default;
FiltersUpdate &FiltersUpdate::operator=(FiltersUpdate &&other) noexcept = default;
FiltersUpdate::~FiltersUpdate() = default;

std::ostream &operator<<(std::ostream &s, const FiltersUpdate &o) {
  return streamf(s, "FiltersUpdate(%o, %o)", o.version_, o.filters_);
}
DEFINE_TYPICAL_JSON(FiltersUpdate, version_, filters_);

std::ostream &operator<<(std::ostream &s, const AckPing &o) {
  return streamf(s, "AckPing(%o)", o.cookie_);
}

DEFINE_TYPICAL_JSON(AckPing, cookie_);

std::ostream &operator<<(std::ostream &s, const GeneralError &o) {
  return streamf(s, "GeneralError(%o)", o.message_);
}
DEFINE_TYPICAL_JSON(GeneralError, message_);
}  // namespace responses

DResponse::DResponse() = default;
DResponse::DResponse(dresponses::AckSyntaxCheck o) : payload_(std::move(o)) {}
DResponse::DResponse(dresponses::AckSubscribe o) : payload_(std::move(o)) {}
DResponse::DResponse(dresponses::AckMoreZgrams o) : payload_(std::move(o)) {}
DResponse::DResponse(dresponses::EstimatesUpdate o) : payload_(std::move(o)) {}
DResponse::DResponse(dresponses::MetadataUpdate o) : payload_(std::move(o)) {}
DResponse::DResponse(dresponses::AckSpecificZgrams o) : payload_(std::move(o)) {}
DResponse::DResponse(dresponses::PlusPlusUpdate o) : payload_(std::move(o)) {}
DResponse::DResponse(dresponses::FiltersUpdate o) : payload_(std::move(o)) {}
DResponse::DResponse(dresponses::AckPing o) : payload_(std::move(o)) {}
DResponse::DResponse(dresponses::GeneralError o) : payload_(std::move(o)) {}
DResponse::DResponse(DResponse &&) noexcept = default;
DResponse &DResponse::operator=(DResponse &&) noexcept = default;
DResponse::~DResponse() = default;

std::ostream &operator<<(std::ostream &s, const DResponse &o) {
  return s << o.payload_;
}

DEFINE_TYPICAL_JSON(DResponse, payload_);
}  // namespace z2kplus::backend::shared::protocol::message
