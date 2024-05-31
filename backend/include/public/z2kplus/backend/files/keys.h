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
// refers to a file in the database. This class is blittable.
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

// Defines a byte range [begin,end) inside a file in the database.
// Typically this is used to locate the [begin, end) of a zgram or metadata record
class IntraFileRange {
public:
  IntraFileRange() = default;
  IntraFileRange(CompressedFileKey fileKey, uint32_t begin, uint32_t end) :
      fileKey_(fileKey), begin_(begin), end_(end) {}

  const CompressedFileKey &fileKey() const { return fileKey_; }
  uint32_t begin() const { return begin_; }
  uint32_t end() const { return end_; }

private:
  // The file being referred to.
  CompressedFileKey fileKey_;
  // The inclusive start of the range.
  uint32_t begin_ = 0;
  // The exclusive end of the range.
  uint32_t end_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const IntraFileRange &o);
};
static_assert(std::is_trivially_copyable_v<IntraFileRange> &&
    std::has_unique_object_representations_v<IntraFileRange>);

// The expanded version of the CompressedFileKey. Less space-efficient but easier
// to work with.
class FileKey {
  using FailFrame = kosak::coding::FailFrame;
public:
  static bool tryCreate(size_t year, size_t month, size_t day, bool logged,
      FileKey *result, const FailFrame &failFrame);

  static bool tryCreate(uint32_t raw, FileKey *result, const FailFrame &failFrame);

//  static constexpr FileKey createUnsafe(size_t year, size_t month, size_t day,
//      bool extra) noexcept {
//    uint32 raw = year;
//    raw = raw * 100 + month;
//    raw = raw * 100 + day;
//    raw = raw * 10 + (extra ? 1 : 0);
//    return FileKey(raw);
//  }

  FileKey() = default;
  ~FileKey() = default;

  size_t year() const;
  size_t month() const;
  size_t day() const;
  bool isLogged() const;

private:
  size_t year_ = 0;
  size_t month_ = 0;
  size_t day_ = 0;
  bool isLogged_ = false;

  friend std::ostream &operator<<(std::ostream &s, const FileKey &o);
};

// FileKey: compressed file key (yyyymmddL)
// file key with offset (yyyymmddL + offset); Location can be our proxy for this
// logged vs unlogged are incomparable
// for more type safety could have separate LoggedFileKey and UnloggedFileKey

// This class is blittable.
class FilePosition {
  using FailFrame = kosak::coding::FailFrame;

public:
  FilePosition() = default;
  FilePosition(FileKey fileKey, uint32_t position) : fileKey_(fileKey), position_(position) {}

  const FileKey &fileKey() const { return fileKey_; }
  uint32_t position() const { return position_; }

private:
  // Which fulltext file this zgram lives in.
  FileKey fileKey_;
  // The character position of start of zgram in the fulltext file.
  uint32_t position_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const FilePosition &o);
};
static_assert(std::is_trivially_copyable_v<FilePosition> &&
    std::has_unique_object_representations_v<FilePosition>);



class InterFileRange {
public:
  InterFileRange() = default;
  InterFileRange(const FileKey &beginKey, uint32_t beginPos,
      const FileKey &endKey, uint32_t endPos);
  InterFileRange(const FilePosition &begin, const FilePosition &end) :
      begin_(begin), end_(end) {}

  const FilePosition &begin() const { return begin_; }
  const FilePosition &end() const { return end_; }

  InterFileRange intersectWith(const InterFileRange &other) const;

  bool empty() const;

private:
  FilePosition begin_;
  FilePosition end_;

  friend std::ostream &operator<<(std::ostream &s, const InterFileRange &o);
};
}  // namespace z2kplus::backend::files
