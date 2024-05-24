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

// This class is blittable.
class FileKey {
  using FailFrame = kosak::coding::FailFrame;
public:
  static bool tryCreate(size_t year, size_t month, size_t day, bool logged,
      FileKey *result, const FailFrame &failFrame);

  static bool tryCreate(uint32_t raw, FileKey *result, const FailFrame &failFrame);

  static constexpr FileKey createUnsafe(size_t year, size_t month, size_t day,
      bool extra) noexcept {
    uint32 raw = year;
    raw = raw * 100 + month;
    raw = raw * 100 + day;
    raw = raw * 100 + (extra ? 1 : 0);
    return FileKey(raw);
  }

  FileKey() = default;
  ~FileKey() = default;

  size_t year() const { return year_; }
  size_t month() const { return month_; }
  size_t day() const { return day_; }
  bool isLogged() const { return isLogged_; }

  uint32_t raw() const { return raw_; }

  int tryCompare(const FileKey &other, const FailFrame &ff) const;

  // Logged keys and unlogged keys are incomparable: attempting to compare them will throw an exception.
  int compare(const FileKey &other) const;


private:
  explicit constexpr FileKey(uint32_t raw) : raw_(raw) {}

  uint32_t raw_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const FileKey &o);
};
static_assert(std::is_trivially_copyable_v<FileKey> && std::has_unique_object_representations_v<FileKey>);


// FileKey: compressed file key (yyyymmddL)
// file key with offset (yyyymmddL + offset); Location can be our proxy for this
// logged vs unlogged are incomparable
// for more type safety could have separate LoggedFileKey and UnloggedFileKey

// This class is blittable.
class FilePosition {
  using FailFrame = kosak::coding::FailFrame;

public:
  Location() = default;
  Location(FileKey fileKey, uint32_t position) : fileKey_(fileKey), position_(position) {}

  const FileKey &fileKey() const { return fileKey_; }
  uint32_t position() const { return position_; }

  int tryCompare(const Location &other, const FailFrame &ff) const;

  int compare(const Location &other) const;
  DEFINE_ALL_COMPARISON_OPERATORS(Location);

private:
  // Which fulltext file this zgram lives in.
  FileKey fileKey_;
  // The character position of start of zgram in the fulltext file.
  uint32_t position_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const Location &o);
};
static_assert(std::is_trivially_copyable_v<FileLocation> && std::has_unique_object_representations_v<FileLocation>);

class IntraFileRange {
public:
  ZgramLocator() = default;
  ZgramLocator(FileKey fileKey, uint32_t position, uint32_t size) : fileKey_(fileKey),
                                                                position_(position), size_(size) {}

  const FileKey &fileKey() const { return fileKey_; }
  uint32_t position() const { return position_; }
  uint32_t size() const { return size_; }

  int compare(const Location &other) const;
  DEFINE_ALL_COMPARISON_OPERATORS(Location);

private:
  // The file being referred to.
  FileKey fileKey_;
  // The inclusive start of the range.
  uint32_t begin_ = 0;
  // The exclusive end of the range.
  uint32_t end_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const FileRange &o);
};
static_assert(std::is_trivially_copyable_v<IntraFileRange> && std::has_unique_object_representations_v<IntraFileRange>);

class InterFileRange {
public:
  ZgramLocator() = default;
  ZgramLocator(FileKey fileKey, uint32_t position, uint32_t size) : fileKey_(fileKey),
  position_(position), size_(size) {}

  const FileKey &fileKey() const { return fileKey_; }
  uint32_t position() const { return position_; }
  uint32_t size() const { return size_; }

  int compare(const Location &other) const;
  DEFINE_ALL_COMPARISON_OPERATORS(Location);

private:
  FilePosition begin_;
  FilePosition end_;

  friend std::ostream &operator<<(std::ostream &s, const FileRange &o);
};
static_assert(std::is_trivially_copyable_v<IntraFileRange> && std::has_unique_object_representations_v<IntraFileRange>);



}  // namespace z2kplus::backend::files
