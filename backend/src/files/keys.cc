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

#include "z2kplus/backend/reverse_index/types.h"

#include <string>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "z2kplus/backend/util/misc.h"

using kosak::coding::streamf;
using kosak::coding::FailFrame;
using kosak::coding::FailRoot;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::files {
namespace {
struct Expander {
  explicit Expander(uint32_t raw) {
    isLogged_ = (raw % 10) != 0;
    raw /= 10;

    day_ = raw % 100;
    raw /= 100;

    month_ = raw % 100;
    raw /= 100;

    year_ = raw;
  }

  size_t year_ = 0;
  size_t month_ = 0;
  size_t day_ = 0;
  bool isLogged_ = false;
};

}  // namespace

bool FileKey::tryCreate(size_t year, size_t month, size_t day, bool logged,
    FileKey *result, const FailFrame &ff) {
  if (year > 2100 || month > 12 || day > 31) {
    return ff.failf(HERE, "At least one of (%o,%o,%o) is suspicious", year, month, day);
  }
  *result = createUnsafe(year, month, day, logged);
  return true;
}

size_t FileKey::year() const {
  return Expander(raw_).year_;
}

size_t FileKey::month() const {
  return Expander(raw_).month_;
}

size_t FileKey::day() const {
  return Expander(raw_).day_;
}

bool FileKey::isLogged() const {
  return Expander(raw_).isLogged_;
}

std::ostream &operator<<(std::ostream &s, const FileKey &o) {
  Expander e(o.raw_);
  // yyyymmdd.{logged,unlogged}
  // Plenty of space
  char buffer[64];
  snprintf(buffer, STATIC_ARRAYSIZE(buffer), "%04zu%02zu%02zu.%s",
      e.year_, e.month_, e.day_, e.isLogged_ ? "logged" : "unlogged");
  s << buffer;
  return s;
}

std::ostream &operator<<(std::ostream &s, const FilePosition &o) {
  streamf(s, "%o:%o", o.fileKey_, o.position_);
  return s;
}

std::ostream &operator<<(std::ostream &s, const IntraFileRange &o) {
  streamf(s, "%o:[%o, %o)", o.fileKey_, o.begin_, o.end_);
  return s;
}

std::ostream &operator<<(std::ostream &s, const InterFileRange &o) {
  streamf(s, "[%o, %o)", o.begin_, o.end_);
  return s;
}
}  // namespace z2kplus::backend::files
