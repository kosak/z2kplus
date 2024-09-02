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
#include <string>
#include <variant>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/myjson.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/shared/protocol/misc.h"

namespace z2kplus::backend::shared::protocol::message {

namespace drequests {
class CheckSyntax {
public:
  CheckSyntax();
  explicit CheckSyntax(std::string query);
  DISALLOW_COPY_AND_ASSIGN(CheckSyntax);
  DECLARE_MOVE_COPY_AND_ASSIGN(CheckSyntax);
  ~CheckSyntax();

  std::string &query() { return query_; }
  const std::string &query() const { return query_; }

private:
  std::string query_;

  friend std::ostream &operator<<(std::ostream &s, const CheckSyntax &o);
  DECLARE_TYPICAL_JSON(CheckSyntax);
};

class Subscribe {
  typedef z2kplus::backend::shared::SearchOrigin SearchOrigin;

public:
  Subscribe();
  Subscribe(std::string &&query, SearchOrigin start, size_t pageSize, size_t queryMargin);
  DISALLOW_COPY_AND_ASSIGN(Subscribe);
  DECLARE_MOVE_COPY_AND_ASSIGN(Subscribe);
  ~Subscribe();

  std::string &query() { return query_; }
  const std::string &query() const { return query_; }
  const SearchOrigin &start() const { return start_; }
  size_t pageSize() const { return pageSize_; }
  size_t queryMargin() const { return queryMargin_; }

private:
  std::string query_;
  SearchOrigin start_;
  size_t pageSize_ = 0;
  size_t queryMargin_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const Subscribe &o);
  DECLARE_TYPICAL_JSON(Subscribe);
};

class GetMoreZgrams {
public:
  GetMoreZgrams();
  GetMoreZgrams(bool forBackSide, uint64_t count);
  DISALLOW_COPY_AND_ASSIGN(GetMoreZgrams);
  DECLARE_MOVE_COPY_AND_ASSIGN(GetMoreZgrams);
  ~GetMoreZgrams();

  bool forBackSide() const {
    return forBackSide_;
  }

  uint64_t count() const {
    return count_;
  }

private:
  // true if requesting more back-side zgrams; false if requesting more front-side zgrams.
  bool forBackSide_ = false;
  // Request at most this many zgrams
  uint64_t count_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const GetMoreZgrams &o);
  DECLARE_TYPICAL_JSON(GetMoreZgrams);
};

class PostZgrams {
public:
  // The zgram, and optionally the zgram it refers to.
  using entry_t = std::pair<ZgramCore, std::optional<ZgramId>>;

  PostZgrams();
  explicit PostZgrams(std::vector<entry_t> entries);
  DISALLOW_COPY_AND_ASSIGN(PostZgrams);
  DECLARE_MOVE_COPY_AND_ASSIGN(PostZgrams);
  ~PostZgrams();

  std::vector<entry_t> &entries() { return entries_; }
  const std::vector<entry_t> &entries() const { return entries_; }

private:
  std::vector<entry_t> entries_;

  friend std::ostream &operator<<(std::ostream &s, const PostZgrams &o);
  DECLARE_TYPICAL_JSON(PostZgrams);
};

class PostMetadata {
public:
  PostMetadata();
  explicit PostMetadata(std::vector<MetadataRecord> metadata);
  DISALLOW_COPY_AND_ASSIGN(PostMetadata);
  DECLARE_MOVE_COPY_AND_ASSIGN(PostMetadata);
  ~PostMetadata();

  std::vector<MetadataRecord> &metadata() { return metadata_; }
  const std::vector<MetadataRecord> &metadata() const { return metadata_; }

private:
  std::vector<MetadataRecord> metadata_;

  friend std::ostream &operator<<(std::ostream &s, const PostMetadata &o);
  DECLARE_TYPICAL_JSON(PostMetadata);
};

class GetSpecificZgrams {
public:
  GetSpecificZgrams();
  GetSpecificZgrams(std::vector<ZgramId> zgramIds);
  DISALLOW_COPY_AND_ASSIGN(GetSpecificZgrams);
  DECLARE_MOVE_COPY_AND_ASSIGN(GetSpecificZgrams);
  ~GetSpecificZgrams();

  std::vector<ZgramId> &zgramIds() { return zgramIds_; }
  const std::vector<ZgramId> &zgramIds() const { return zgramIds_; }

private:
  std::vector<ZgramId> zgramIds_;

  friend std::ostream &operator<<(std::ostream &s, const GetSpecificZgrams &o);
  DECLARE_TYPICAL_JSON(GetSpecificZgrams);
};

class ProposeFilters {
public:
  ProposeFilters();
  ProposeFilters(uint64_t basedOnVersion, bool theseFiltersAreNew, std::vector<Filter> filters);
  DISALLOW_COPY_AND_ASSIGN(ProposeFilters);
  DECLARE_MOVE_COPY_AND_ASSIGN(ProposeFilters);
  ~ProposeFilters();

private:
  uint64_t basedOnVersion_ = 0;
  /**
   * False means existing filters, i.e. saved in the browser session.
   * True means new filters, i.e. just entered by the user
   */
  bool theseFiltersAreNew_ = false;
  std::vector<Filter> filters_;

  friend std::ostream &operator<<(std::ostream &s, const ProposeFilters &o);
  DECLARE_TYPICAL_JSON(ProposeFilters);
};

class Ping {
public:
  Ping() = default;
  explicit Ping(size_t cookie) : cookie_(cookie) {}
  DISALLOW_COPY_AND_ASSIGN(Ping);
  DEFINE_MOVE_COPY_AND_ASSIGN(Ping);
  ~Ping() = default;

  [[nodiscard]] size_t cookie() const { return cookie_; }

private:
  size_t cookie_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const Ping &o);
  DECLARE_TYPICAL_JSON(Ping);
};

typedef std::variant<CheckSyntax, Subscribe, GetMoreZgrams, PostZgrams, PostMetadata, GetSpecificZgrams,
  ProposeFilters, Ping> payload_t;

DECLARE_VARIANT_JSON(DRequestPayloadHolder, payload_t,
    ("CheckSyntax", "Subscribe", "GetMoreZgrams", "PostZgrams", "PostMetadata", "GetSpecificZgrams",
        "ProposeFilters", "Ping"));
}  // namespace drequests

class DRequest {
  typedef kosak::coding::FailFrame FailFrame;

public:
  DRequest();
  explicit DRequest(drequests::CheckSyntax o);
  explicit DRequest(drequests::Subscribe o);
  explicit DRequest(drequests::GetMoreZgrams o);
  explicit DRequest(drequests::PostZgrams o);
  explicit DRequest(drequests::PostMetadata o);
  explicit DRequest(drequests::GetSpecificZgrams o);
  explicit DRequest(drequests::ProposeFilters o);
  explicit DRequest(drequests::Ping o);
  DISALLOW_COPY_AND_ASSIGN(DRequest);
  DECLARE_MOVE_COPY_AND_ASSIGN(DRequest);
  ~DRequest();

  [[nodiscard]] drequests::payload_t &payload() { return payload_; }
  [[nodiscard]] const drequests::payload_t &payload() const { return payload_; }

private:
  drequests::payload_t payload_;

  friend std::ostream &operator<<(std::ostream &s, const DRequest &o) {
    return s << o.payload_;
  }

  DECLARE_TYPICAL_JSON(DRequest);
};
}  // namespace z2kplus::backend::shared::protocol::message
