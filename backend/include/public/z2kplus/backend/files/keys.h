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

#include <chrono>
#include <ctime>
#include <functional>
#include <iostream>
#include <limits>
#include <regex>
#include <string>
#include <string_view>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/comparers.h"
#include "kosak/coding/failures.h"

namespace z2kplus::backend::files {

// Defines a key (yyyy, mm, dd, {logged=true, unlogged=false}) that uniquely
// refers to a file in the database.
// This is blitted inside the index.
class CompressedFileKey {
  using FailFrame = kosak::coding::FailFrame;
public:
  CompressedFileKey() = default;
  explicit constexpr CompressedFileKey(uint32_t raw) : raw_(raw) {}

  uint32_t raw() const { return raw_; }

private:
  uint32_t raw_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const CompressedFileKey &o);
};

static_assert(std::is_trivially_copyable_v<CompressedFileKey> &&
    std::has_unique_object_representations_v<CompressedFileKey>);

// A compressed file key, but tagged with the Logged boolean, in order to participate
// in the more typesafe data structures below.
template<bool IsLogged>
class TaggedFileKey {
public:
  TaggedFileKey() : key_(IsLogged ? 1 : 0) {}
  explicit TaggedFileKey(CompressedFileKey key) : key_(key) {}

  const CompressedFileKey &key() const { return key_; }
  uint32_t raw() const { return key_.raw(); }

private:
  CompressedFileKey key_;

  friend bool operator<(const TaggedFileKey &lhs, const TaggedFileKey &rhs);

  friend std::ostream &operator<<(std::ostream &s, const TaggedFileKey &o);
};
static_assert(std::is_trivially_copyable_v<TaggedFileKey<false>> &&
    std::has_unique_object_representations_v<TaggedFileKey<false>>);
static_assert(std::is_trivially_copyable_v<TaggedFileKey<true>> &&
    std::has_unique_object_representations_v<TaggedFileKey<true>>);

// The expanded version of the CompressedFileKey. Less space-efficient but easier
// to work with.
class ExpandedFileKey {
  using FailFrame = kosak::coding::FailFrame;
public:
  static bool tryCreate(uint32_t year, uint32_t month, uint32_t day, bool logged,
      ExpandedFileKey *result, const FailFrame &failFrame);

  static constexpr ExpandedFileKey createUnsafe(uint32_t year, uint32_t month, uint32_t day,
      bool isLogged) noexcept {
    return {year, month, day, isLogged};
  }

  ExpandedFileKey(std::chrono::system_clock::time_point now, bool isLogged);

  ExpandedFileKey() = default;
  explicit ExpandedFileKey(CompressedFileKey cfk);

  CompressedFileKey compress() const;
  std::pair<std::optional<TaggedFileKey<true>>, std::optional<TaggedFileKey<false>>> visit() const;

  size_t year() const { return year_; }
  size_t month() const { return month_; }
  size_t day() const { return day_; }
  bool isLogged() const { return isLogged_; }

private:
  constexpr ExpandedFileKey(uint32_t year, uint32_t month, uint32_t day, bool isLogged) noexcept :
    year_(year), month_(month), day_(day), isLogged_(isLogged) {}

  uint32_t year_ = 0;
  uint32_t month_ = 0;
  uint32_t day_ = 0;
  bool isLogged_ = false;

  friend std::ostream &operator<<(std::ostream &s, const ExpandedFileKey &o);
};

// Defines the start and end of a zgram or metadata record, either logged or unlogged.
// It is used inside ZgramInfo. This class is blittable.
class LogLocation {
public:
  LogLocation() = default;
  LogLocation(CompressedFileKey fileKey, uint32_t begin, uint32_t end) :
      fileKey_(fileKey), begin_(begin), end_(end) {}

  const CompressedFileKey &fileKey() const { return fileKey_; }
  uint32_t begin() const { return begin_; }
  uint32_t end() const { return end_; }

private:
  // Which fulltext file this zgram lives in.
  CompressedFileKey fileKey_;
  // The character position of start of the zgram in the fulltext file.
  uint32_t begin_ = 0;
  // The character position of the end of the zgram in the fulltext file.
  uint32_t end_ = 0;

  [[maybe_unused]]
  uint32_t padding_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const LogLocation &zg);
};
static_assert(std::is_trivially_copyable_v<LogLocation> &&
  std::has_unique_object_representations_v<LogLocation>);


// Defines a position inside a file. This is used in the FrozenIndex to keep
// track of the end of the logged and unlogged databases at the time the
// FrozenIndex was created. It is templated for extra programming safety.
// This class is blittable.
template<bool IsLogged>
class FilePosition {
  using FailFrame = kosak::coding::FailFrame;

public:
  FilePosition(TaggedFileKey<IsLogged> fileKey, uint32_t position)
      : fileKey_(fileKey), position_(position) {}
  FilePosition() = default;

  const TaggedFileKey<IsLogged> &fileKey() const { return fileKey_; }
  uint32_t position() const { return position_; }

private:
  // Which fulltext file this zgram lives in.
  TaggedFileKey<IsLogged> fileKey_;
  // The character position of start of zgram in the fulltext file.
  uint32_t position_ = 0;

  friend bool operator<(const FilePosition &lhs, const FilePosition &rhs);

  friend std::ostream &operator<<(std::ostream &s, const FilePosition &o) {
    return kosak::coding::streamf(s, "%o:%o", o.fileKey_, o.position_);
  }
};

static_assert(std::is_trivially_copyable_v<FilePosition<false>> &&
    std::has_unique_object_representations_v<FilePosition<false>>);
static_assert(std::is_trivially_copyable_v<FilePosition<true>> &&
    std::has_unique_object_representations_v<FilePosition<true>>);

// Defines a byte range [begin,end) inside a file in the database.
template<bool IsLogged>
class IntraFileRange {
public:
  IntraFileRange() = default;
  IntraFileRange(TaggedFileKey<IsLogged> fileKey, uint32_t begin, uint32_t end) :
      fileKey_(fileKey), begin_(begin), end_(end) {}

  const auto &fileKey() const { return fileKey_; }
  uint32_t begin() const { return begin_; }
  uint32_t end() const { return end_; }

private:
  // The file being referred to.
  TaggedFileKey<IsLogged> fileKey_;
  // The inclusive start of the range.
  uint32_t begin_ = 0;
  // The exclusive end of the range.
  uint32_t end_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const IntraFileRange &o) {
    return kosak::coding::streamf(s, "%o:[%o-%o)", o.fileKey_, o.begin_, o.end_);
  }
};
static_assert(std::is_trivially_copyable_v<IntraFileRange<false>> &&
    std::has_unique_object_representations_v<IntraFileRange<false>>);
static_assert(std::is_trivially_copyable_v<IntraFileRange<true>> &&
    std::has_unique_object_representations_v<IntraFileRange<true>>);

// Defines a range across files. This is used as a parameter to define the whole
// valid range of {logged, unlogged} zgrams for a method to look at.
template<bool IsLogged>
class InterFileRange {
public:
  InterFileRange(TaggedFileKey<IsLogged> beginKey, uint32_t beginPos,
      TaggedFileKey<IsLogged> endKey, uint32_t endPos) :
      begin_(beginKey, beginPos), end_(endKey, endPos) {}

  InterFileRange(FilePosition<IsLogged> begin, FilePosition<IsLogged> end) :
    begin_(begin), end_(end) {}

  InterFileRange() = default;
//  InterFileRange(const FileKey &beginKey, uint32_t beginPos,
//      const FileKey &endKey, uint32_t endPos);
//  InterFileRange(const FilePosition &begin, const FilePosition &end) :
//      begin_(begin), end_(end) {}

  const FilePosition<IsLogged> &begin() const { return begin_; }
  const FilePosition<IsLogged> &end() const { return end_; }

  InterFileRange intersectWith(const InterFileRange &other) const;

  bool empty() const;

private:
  FilePosition<IsLogged> begin_;
  FilePosition<IsLogged> end_;

  friend std::ostream &operator<<(std::ostream &s, const InterFileRange &o) {
    return kosak::coding::streamf(s, "[%o--%o)", o.begin_, o.end_);
  }
};
}  // namespace z2kplus::backend::files
