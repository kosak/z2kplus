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

Post::Post() = default;
Post::Post(std::vector<ZgramCore> zgrams, std::vector<MetadataRecord> metadata) :
    zgrams_(std::move(zgrams)), metadata_(std::move(metadata)) {}
Post::Post(Post &&) noexcept = default;
Post &Post::operator=(Post &&) noexcept = default;
Post::~Post() = default;

std::ostream &operator<<(std::ostream &s, const Post &o) {
  return streamf(s, "Post(zgrams=%o\nmetadata=%o)", o.zgrams_, o.metadata_);
}
DEFINE_TYPICAL_JSON(Post, zgrams_, metadata_);

std::ostream &operator<<(std::ostream &s, const Ping &o) {
  return streamf(s, "Ping(%o)", o.cookie_);
}
DEFINE_TYPICAL_JSON(Ping, cookie_);

DEFINE_VARIANT_JSON(DRequestPayloadHolder, drequestPayload_t);
}  // namespace drequests

DRequest::DRequest() = default;
DRequest::DRequest(drequests::CheckSyntax &&o) : payload_(std::move(o)) {}
DRequest::DRequest(drequests::Subscribe &&o) : payload_(std::move(o)) {}
DRequest::DRequest(drequests::GetMoreZgrams &&o) : payload_(std::move(o)) {}
DRequest::DRequest(drequests::Post &&o) : payload_(std::move(o)) {}
DRequest::DRequest(drequests::Ping &&o) : payload_(std::move(o)) {}
DRequest::DRequest(DRequest &&) noexcept = default;
DRequest &DRequest::operator=(DRequest &&) noexcept = default;
DRequest::~DRequest() = default;

DEFINE_TYPICAL_JSON(DRequest, payload_);
}  // namespace z2kplus::backend::shared::protocol
