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
enum class FileKeyKind { Logged, Unlogged, Either };

namespace internal {
uint32_t timePointToRaw(std::chrono::system_clock::time_point timePoint, bool isLogged);
}  // namespace internal

// Defines a key (yyyy, mm, dd, {logged=true, unlogged=false}) that uniquely
// refers to a file in the database.
// This is blitted inside the index.
// For extra programming safety, is templated into one of three partitions:
// Logged, Unlogged, Either
template<FileKeyKind Kind>
class FileKey {
private:
  explicit constexpr FileKey(uint32_t raw) : raw_(raw) {}

public:
  static FileKey infinity;

  static constexpr FileKey createUnsafe(uint32_t year, uint32_t month, uint32_t day, bool isLogged) {
    auto raw = year;
    raw = raw * 100 + month;
    raw = raw * 100 + day;
    raw = raw * 10 + (isLogged ? 1 : 0);
    return FileKey(raw);
  }

  static constexpr FileKey createUnsafe(uint32_t raw) {
    if (Kind == FileKeyKind::Logged && ((raw & 1) == 0)) {
      throw std::runtime_error("raw value not Logged kind");
    }
    if (Kind == FileKeyKind::Unlogged && ((raw & 1) == 1)) {
      throw std::runtime_error("raw value not Unlogged kind");
    }
    FileKey<Kind> temp(raw);
    return temp;
  }

  static FileKey createFromTimePoint(std::chrono::system_clock::time_point timePoint) {
    static_assert(Kind != FileKeyKind::Either);
    auto raw = internal::timePointToRaw(timePoint, Kind == FileKeyKind::Logged);
    return FileKey(raw);
  }

  constexpr FileKey() : raw_(Kind == FileKeyKind::Logged ? 1 : 0) {
  }

  template<FileKeyKind OtherKind>
  constexpr FileKey(FileKey<OtherKind> other) : raw_(other.raw()) {
    static_assert(Kind == FileKeyKind::Either);
  }

  constexpr std::pair<std::optional<FileKey<FileKeyKind::Logged>>, std::optional<FileKey<FileKeyKind::Unlogged>>> visit() const {
    if constexpr (Kind == FileKeyKind::Logged) {
      return {*this, {}};
    } else if constexpr (Kind == FileKeyKind::Unlogged) {
      return {{}, *this};
    } else if constexpr (Kind == FileKeyKind::Either) {
      if (isLogged()) {
        return {FileKey<FileKeyKind::Logged>::createUnsafe(raw_), {}};
      } else {
        return {{}, FileKey<FileKeyKind::Unlogged>::createUnsafe(raw_)};
      }
    } else {
      throw std::runtime_error("Impossible: unexpected case");
    }
  }

  std::tuple<uint32, uint32, uint32, bool> expand() const {
    auto temp = raw_;
    bool isLogged = (temp % 10) != 0;
    temp /= 10;

    uint32_t day = temp % 100;
    temp /= 100;

    uint32_t month = temp % 100;
    temp /= 100;

    uint32_t year = temp;
    return std::make_tuple(year, month, day, isLogged);
  }

  uint32_t raw() const { return raw_; }

  bool isLogged() const {
    return (raw_ & 1) != 0;
  }

private:
  uint32_t raw_ = 0;

  friend bool operator<(const FileKey &lhs, const FileKey &rhs) {
    static_assert(Kind != FileKeyKind::Either);
    return lhs.raw_ < rhs.raw_;
  }

  friend bool operator==(const FileKey &lhs, const FileKey &rhs) {
    return lhs.raw_ == rhs.raw_;
  }

  friend std::ostream &operator<<(std::ostream &s, const FileKey &o) {
    // yyyymmdd.{logged,unlogged}
    // Plenty of space
    char buffer[64];
    auto [y, m, d, l] = o.expand();
    snprintf(buffer, STATIC_ARRAYSIZE(buffer), "%04u%02u%02u.%s",
        y, m, d, l ? "logged" : "unlogged");
    return s << buffer;
  }
};

static_assert(std::is_trivially_copyable_v<FileKey<FileKeyKind::Either>> &&
    std::has_unique_object_representations_v<FileKey<FileKeyKind::Either>>);

template<FileKeyKind Kind>
FileKey<Kind> FileKey<Kind>::infinity = FileKey<Kind>::createUnsafe(9999, 12, 31, Kind == FileKeyKind::Logged);

// Defines the start and end of a zgram or metadata record, either logged or unlogged.
// It is used inside ZgramInfo. This class is blittable.
class LogLocation {
public:
  LogLocation() = default;
  LogLocation(FileKey<FileKeyKind::Either> fileKey, uint32_t offset, uint32_t size, const char *zamboniTime) :
      fileKey_(fileKey), offset_(offset), size_(size) {}

  const FileKey<FileKeyKind::Either> &fileKey() const { return fileKey_; }
  uint32_t offset() const { return offset_; }
  uint32_t size() const { return size_; }

private:
  // Which fulltext file this zgram lives in.
  FileKey<FileKeyKind::Either> fileKey_;
  // The character position of start of the record in the fulltext file.
  uint32_t offset_ = 0;
  // The number of bytes in this record
  uint32_t size_ = 0;

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
template<FileKeyKind Kind>
class FilePosition {
  using FailFrame = kosak::coding::FailFrame;

public:
  static FilePosition zero;
  static FilePosition infinity;

  constexpr FilePosition(FileKey<Kind> fileKey, uint32_t position)
      : fileKey_(fileKey), position_(position) {}
  constexpr FilePosition() = default;

  template<FileKeyKind OtherKind>
  constexpr FilePosition(FilePosition<OtherKind> other) : fileKey_(other.fileKey()),
    position_(other.position()) {
    static_assert(Kind == FileKeyKind::Either);
  }

  const FileKey<Kind> &fileKey() const { return fileKey_; }
  uint32_t position() const { return position_; }

private:
  // Which fulltext file this zgram lives in.
  FileKey<Kind> fileKey_;
  // The character position of start of zgram in the fulltext file.
  uint32_t position_ = 0;

  friend bool operator<(const FilePosition &lhs, const FilePosition &rhs) {
    static_assert(Kind != FileKeyKind::Either);
    if (lhs.fileKey_ < rhs.fileKey_) {
      return true;
    }
    if (rhs.fileKey_ < lhs.fileKey_) {
      return false;
    }
    return lhs.position_ < rhs.position_;
  }

  friend bool operator==(const FilePosition &lhs, const FilePosition &rhs) {
    return lhs.fileKey_ == rhs.fileKey_ && lhs.position_ == rhs.position_;
  }

  friend std::ostream &operator<<(std::ostream &s, const FilePosition &o) {
    return kosak::coding::streamf(s, "%o:%o", o.fileKey_, o.position_);
  }
};
static_assert(std::is_trivially_copyable_v<FilePosition<FileKeyKind::Either>> &&
    std::has_unique_object_representations_v<FilePosition<FileKeyKind::Either>>);

template<FileKeyKind Kind>
FilePosition<Kind> FilePosition<Kind>::zero;

template<FileKeyKind Kind>
FilePosition<Kind> FilePosition<Kind>::infinity(FileKey<Kind>::infinity, 0);

// Defines a byte range [begin,end) inside a file in the database.
template<FileKeyKind Kind>
class IntraFileRange {
public:
  IntraFileRange() = default;
  IntraFileRange(FileKey<Kind> fileKey, uint32_t begin, uint32_t end) :
      fileKey_(fileKey), begin_(begin), end_(end) {}

  template<FileKeyKind OtherKind>
  IntraFileRange(const IntraFileRange<OtherKind> &other) :
    fileKey_(other.fileKey()), begin_(other.begin()), end_(other.end()) {
    static_assert(Kind == FileKeyKind::Either);
  }

  const FileKey<Kind> &fileKey() const { return fileKey_; }
  uint32_t begin() const { return begin_; }
  uint32_t end() const { return end_; }

private:
  // The file being referred to.
  FileKey<Kind> fileKey_;
  // The inclusive start of the range.
  uint32_t begin_ = 0;
  // The exclusive end of the range.
  uint32_t end_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const IntraFileRange &o) {
    return kosak::coding::streamf(s, "%o:[%o-%o)", o.fileKey_, o.begin_, o.end_);
  }
};
static_assert(std::is_trivially_copyable_v<IntraFileRange<FileKeyKind::Either>> &&
    std::has_unique_object_representations_v<IntraFileRange<FileKeyKind::Either>>);

// Defines a range across files. This is used as a parameter to define the whole
// valid range of {logged, unlogged} zgrams for a method to look at.
template<FileKeyKind Kind>
class InterFileRange {
public:
  static InterFileRange everything;

  constexpr InterFileRange(FileKey<Kind> beginKey, uint32_t beginPos,
      FileKey<Kind> endKey, uint32_t endPos) :
      begin_(beginKey, beginPos), end_(endKey, endPos) {}

  constexpr InterFileRange(FilePosition<Kind> begin, FilePosition<Kind> end) :
    begin_(begin), end_(end) {}

  InterFileRange() = default;
//  InterFileRange(const FileKey &beginKey, uint32_t beginPos,
//      const FileKey &endKey, uint32_t endPos);
//  InterFileRange(const FilePosition &begin, const FilePosition &end) :
//      begin_(begin), end_(end) {}

  const FilePosition<Kind> &begin() const { return begin_; }
  const FilePosition<Kind> &end() const { return end_; }

  InterFileRange intersectWith(const InterFileRange &other) const {
    auto newBegin = std::max(begin_, other.begin_);
    auto newEnd = std::min(end_, other.end_);
    if (newEnd < newBegin) {
      // Ranges don't intersect; return empty.
      return {end_, end_};
    }
    return {newBegin, newEnd};
  }

  bool empty() const {
    return begin_ == end_;
  }

private:
  FilePosition<Kind> begin_;
  FilePosition<Kind> end_;

  friend std::ostream &operator<<(std::ostream &s, const InterFileRange &o) {
    return kosak::coding::streamf(s, "[%o--%o)", o.begin_, o.end_);
  }
};

template<FileKeyKind Kind>
InterFileRange<Kind> InterFileRange<Kind>::everything(FilePosition<Kind>::zero,
    FilePosition<Kind>::infinity);
}  // namespace z2kplus::backend::files
