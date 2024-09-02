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

#include <iostream>
#include "z2kplus/backend/shared/protocol/message/drequest.h"
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/myjson.h"

using kosak::coding::ParseContext;
using kosak::coding::streamf;

namespace z2kplus::backend::shared::protocol::message {

namespace drequests {

CheckSyntax::CheckSyntax() = default;
CheckSyntax::CheckSyntax(std::string query) : query_(std::move(query)) {}
CheckSyntax::CheckSyntax(CheckSyntax &&) noexcept = default;
CheckSyntax &CheckSyntax::operator=(CheckSyntax &&) noexcept = default;
CheckSyntax::~CheckSyntax() = default;

std::ostream &operator<<(std::ostream &s, const CheckSyntax &o) {
  return streamf(s, "CheckSyntax(%o)", o.query_);
}

DEFINE_TYPICAL_JSON(CheckSyntax, query_);

Subscribe::Subscribe() = default;
Subscribe::Subscribe(std::string &&query, SearchOrigin start, size_t pageSize, size_t queryMargin) :
    query_(std::move(query)), start_(start), pageSize_(pageSize), queryMargin_(queryMargin) {}
Subscribe::Subscribe(Subscribe &&) noexcept = default;
Subscribe &Subscribe::operator=(Subscribe &&) noexcept = default;
Subscribe::~Subscribe() = default;

std::ostream &operator<<(std::ostream &s, const Subscribe &o) {
  return streamf(s, "Subscribe(%o,%o,%o,%o)", o.query_, o.start_, o.start_, o.pageSize_,
      o.queryMargin_);
}

DEFINE_TYPICAL_JSON(Subscribe, query_, start_, pageSize_, queryMargin_);

GetMoreZgrams::GetMoreZgrams() = default;
GetMoreZgrams::GetMoreZgrams(bool forBackSide, uint64_t count) : forBackSide_(forBackSide),
    count_(count) {}
GetMoreZgrams::GetMoreZgrams(GetMoreZgrams &&) noexcept = default;
GetMoreZgrams &GetMoreZgrams::operator=(GetMoreZgrams &&) noexcept = default;
GetMoreZgrams::~GetMoreZgrams() = default;

std::ostream &operator<<(std::ostream &s, const GetMoreZgrams &o) {
  return streamf(s, "GetMore(%o,%o)", o.forBackSide_, o.count_);
}

DEFINE_TYPICAL_JSON(GetMoreZgrams, forBackSide_, count_);

PostZgrams::PostZgrams() = default;
PostZgrams::PostZgrams(std::vector<entry_t> entries) : entries_(std::move(entries)) {}
PostZgrams::PostZgrams(PostZgrams &&) noexcept = default;
PostZgrams &PostZgrams::operator=(PostZgrams &&) noexcept = default;
PostZgrams::~PostZgrams() = default;

std::ostream &operator<<(std::ostream &s, const PostZgrams &o) {
  return streamf(s, "PostZgrams(entries=%o)", o.entries_);
}
DEFINE_TYPICAL_JSON(PostZgrams, entries_);

PostMetadata::PostMetadata() = default;
PostMetadata::PostMetadata(std::vector<MetadataRecord> metadata) : metadata_(std::move(metadata)) {}
PostMetadata::PostMetadata(PostMetadata &&) noexcept = default;
PostMetadata &PostMetadata::operator=(PostMetadata &&) noexcept = default;
PostMetadata::~PostMetadata() = default;

std::ostream &operator<<(std::ostream &s, const PostMetadata &o) {
  return streamf(s, "PostMetadata(md=%o)", o.metadata_);
}
DEFINE_TYPICAL_JSON(PostMetadata, metadata_);

GetSpecificZgrams::GetSpecificZgrams() = default;
GetSpecificZgrams::GetSpecificZgrams(std::vector<ZgramId> zgramIds) : zgramIds_(std::move(zgramIds)) {}
GetSpecificZgrams::GetSpecificZgrams(GetSpecificZgrams &&) noexcept = default;
GetSpecificZgrams &GetSpecificZgrams::operator=(GetSpecificZgrams &&) noexcept = default;
GetSpecificZgrams::~GetSpecificZgrams() = default;

std::ostream &operator<<(std::ostream &s, const GetSpecificZgrams &o) {
  return streamf(s, "GetSpecificZgrams(zgramIds=%o)", o.zgramIds_);
}
DEFINE_TYPICAL_JSON(GetSpecificZgrams, zgramIds_);

ProposeFilters::ProposeFilters() = default;
ProposeFilters::ProposeFilters(uint64_t basedOnVersion, bool theseFiltersAreNew, std::vector<Filter> filters) :
  basedOnVersion_(basedOnVersion), theseFiltersAreNew_(theseFiltersAreNew), filters_(std::move(filters)) {}
ProposeFilters::ProposeFilters(ProposeFilters &&) noexcept = default;
ProposeFilters &ProposeFilters::operator=(ProposeFilters &&) noexcept = default;
ProposeFilters::~ProposeFilters() = default;

std::ostream &operator<<(std::ostream &s, const ProposeFilters &o) {
  return streamf(s, "ProposeFilters(%o, %o, %o)", o.basedOnVersion_, o.theseFiltersAreNew_, o.filters_);
}
DEFINE_TYPICAL_JSON(ProposeFilters, basedOnVersion_, theseFiltersAreNew_, filters_);

std::ostream &operator<<(std::ostream &s, const Ping &o) {
  return streamf(s, "Ping(%o)", o.cookie_);
}
DEFINE_TYPICAL_JSON(Ping, cookie_);

DEFINE_VARIANT_JSON(DRequestPayloadHolder, drequestPayload_t);
}  // namespace drequests

DRequest::DRequest() = default;
DRequest::DRequest(drequests::CheckSyntax o) : payload_(std::move(o)) {}
DRequest::DRequest(drequests::Subscribe o) : payload_(std::move(o)) {}
DRequest::DRequest(drequests::GetMoreZgrams o) : payload_(std::move(o)) {}
DRequest::DRequest(drequests::PostZgrams o) : payload_(std::move(o)) {}
DRequest::DRequest(drequests::PostMetadata o) : payload_(std::move(o)) {}
DRequest::DRequest(drequests::GetSpecificZgrams o) : payload_(std::move(o)) {}
DRequest::DRequest(drequests::Ping o) : payload_(std::move(o)) {}
DRequest::DRequest(DRequest &&) noexcept = default;
DRequest &DRequest::operator=(DRequest &&) noexcept = default;
DRequest::~DRequest() = default;

DEFINE_TYPICAL_JSON(DRequest, payload_);
}  // namespace z2kplus::backend::shared::protocol
