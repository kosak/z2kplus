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
class ExpandedFileKey;

// This class is blittable.
namespace internal {
class FileKeyCore {
  using FailFrame = kosak::coding::FailFrame;
public:
  static constexpr size_t yearBegin = 1970;
  static constexpr size_t yearEnd = 2101;
  static constexpr size_t monthBegin = 1;
  static constexpr size_t monthEnd = 13;
  static constexpr size_t dayBegin = 1;
  static constexpr size_t dayEnd = 32;
  static constexpr size_t partBegin = 0;
  static constexpr size_t partEnd = 1000;
  static constexpr size_t extraBegin = 0;
  static constexpr size_t extraEnd = 2;

  static constexpr size_t yearWidth = yearEnd - yearBegin;
  static constexpr size_t monthWidth = monthEnd - monthBegin;
  static constexpr size_t dayWidth = dayEnd - dayBegin;
  static constexpr size_t partWidth = partEnd - partBegin;
  static constexpr size_t extraWidth = extraEnd - extraBegin;

  static constexpr size_t totalWidth = yearWidth * monthWidth * dayWidth * partWidth * extraWidth;
  static_assert(totalWidth < std::numeric_limits<uint32_t>::max());

  static bool tryCreate(size_t year, size_t month, size_t day, size_t part, bool extra,
      FileKeyCore *result, const FailFrame &failFrame);

  static bool tryCreate(uint32_t raw, FileKeyCore *result, const FailFrame &failFrame);

  static constexpr FileKeyCore createUnsafe(size_t year, size_t month, size_t day, size_t part,
      bool extra) noexcept {
    uint32 raw = year - yearBegin;
    raw = raw * monthWidth + (month - monthBegin);
    raw = raw * dayWidth + (day - dayBegin);
    raw = raw * partWidth + (part - partBegin);
    raw = raw * extraWidth + (extra ? 1 : 0);
    return FileKeyCore(raw);
  }

  FileKeyCore() = default;
  ~FileKeyCore() = default;

  ExpandedFileKey asExpandedFileKey() const;

  uint32_t raw() const { return raw_; }

  int compare(const FileKeyCore &other) const {
    return kosak::coding::compare(&raw_, &other.raw_);
  }

  DEFINE_ALL_COMPARISON_OPERATORS(FileKeyCore);

  void dump(std::ostream &s, char extraWhenFalse, char extraWhenTrue) const;

private:
  explicit constexpr FileKeyCore(uint32_t raw) : raw_(raw) {}

  uint32_t raw_ = 0;

};
}  // namespace internal

class ExpandedFileKey {
public:
  size_t year() const { return year_; }
  size_t month() const { return month_; }
  size_t day() const { return day_; }
  size_t part() const { return part_; }
  bool isLogged() const { return isLogged_; }

  void dump(std::ostream &s, char extraWhenFalse, char extraWhenTrue) const;

private:
  ExpandedFileKey(size_t year, size_t month, size_t day, size_t part, bool isLogged);

  size_t year_ = 0;
  size_t month_ = 0;
  size_t day_ = 0;
  size_t part_ = 0;
  bool isLogged_ = false;

  friend std::ostream &operator<<(std::ostream &s, const ExpandedFileKey &o);
  friend class internal::FileKeyCore;
};

// This class is blittable.
class FileKey;
class DateAndPartKey {
  using FailFrame = kosak::coding::FailFrame;
public:
  static bool tryCreate(size_t year, size_t month, size_t day, size_t part, DateAndPartKey *result,
      const FailFrame &ff);

  static bool tryCreate(std::chrono::system_clock::time_point p, DateAndPartKey *result,
      const FailFrame &ff);

  static constexpr DateAndPartKey createUnsafe(size_t year, size_t month, size_t day, size_t part)
      noexcept {
    return DateAndPartKey(internal::FileKeyCore::createUnsafe(year, month, day, part, false));
  }

  DateAndPartKey() = default;
  ~DateAndPartKey() = default;

  ExpandedFileKey asExpandedFileKey() const {
    return core_.asExpandedFileKey();
  }

  FileKey asFileKey(bool logged) const;

  bool tryBump(DateAndPartKey *result, const FailFrame &ff) const;

  bool thisOrNow(std::chrono::system_clock::time_point now, DateAndPartKey *result,
      const FailFrame &ff) const;

  uint32_t raw() const { return core_.raw(); }

  int compare(const DateAndPartKey &other) const { return core_.compare(other.core_); }
  DEFINE_ALL_COMPARISON_OPERATORS(DateAndPartKey);

private:
  explicit constexpr DateAndPartKey(const internal::FileKeyCore &core) : core_(core) {}

  internal::FileKeyCore core_;

  friend std::ostream &operator<<(std::ostream &s, const DateAndPartKey &o) {
    o.core_.dump(s, 0, 0);
    return s;
  }
};

// This class is blittable.
class FileKey {
  using FailFrame = kosak::coding::FailFrame;
public:
  static bool tryCreate(size_t year, size_t month, size_t day, size_t part, bool isLogged,
      FileKey *result, const FailFrame &failFrame);
  static bool tryCreate(uint32_t raw, FileKey *result, const FailFrame &failFrame);

  static constexpr FileKey createUnsafe(size_t year, size_t month, size_t day, size_t part,
      bool logged) noexcept {
    return FileKey(internal::FileKeyCore::createUnsafe(year, month, day, part, logged));
  }

  FileKey() = default;
  ~FileKey() = default;

  ExpandedFileKey asExpandedFileKey() const {
    return core_.asExpandedFileKey();
  }

  DateAndPartKey asDateAndPartKey() const;

  uint32_t raw() const { return core_.raw(); }

  int compare(const FileKey &other) const { return core_.compare(other.core_); }
  DEFINE_ALL_COMPARISON_OPERATORS(FileKey);

private:
  explicit constexpr FileKey(const internal::FileKeyCore &core) : core_(core) {}
  internal::FileKeyCore core_;

  friend std::ostream &operator<<(std::ostream &s, const FileKey &o) {
    o.core_.dump(s, 'U', 'L');
    return s;
  }
};


// This class is blittable.
class Location {
public:
  Location() = default;
  Location(FileKey fileKey, uint32_t position, uint32_t size) : fileKey_(fileKey),
    position_(position), size_(size) {}

  const FileKey &fileKey() const { return fileKey_; }
  uint32_t position() const { return position_; }
  uint32_t size() const { return size_; }

  int compare(const Location &other) const;
  DEFINE_ALL_COMPARISON_OPERATORS(Location);

private:
  // Which fulltext file this zgram lives in.
  FileKey fileKey_;
  // The character position of start of zgram in the fulltext file.
  uint32_t position_ = 0;
  // The length of zgram in characters.
  uint32_t size_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const Location &o);
};
}  // namespace z2kplus::backend::files
