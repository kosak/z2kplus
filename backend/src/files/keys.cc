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

ExpandedFileKey::ExpandedFileKey(size_t year, size_t month, size_t day, size_t part,
    bool isLogged) : year_(year), month_(month), day_(day), part_(part), isLogged_(isLogged) {}

void ExpandedFileKey::dump(std::ostream &s, char extraWhenLogged, char extraWhenUnlogged) const {
  // If the client passes in 0 for either arg, then these will be the empty string
  const char extraLoggedString[2] = {extraWhenLogged, 0};
  const char extraUnloggedString[2] = {extraWhenUnlogged, 0};

  // yyyymmdd.NNN[L/U]
  // Plenty of space
  char buffer[32];
  snprintf(buffer, STATIC_ARRAYSIZE(buffer), "%04zu%02zu%02zu.%03zu%s",
      year_, month_, day_, part_, isLogged_ ? extraLoggedString : extraUnloggedString);
  s << buffer;
}

std::ostream &operator<<(std::ostream &s, const ExpandedFileKey &o) {
  o.dump(s, 'L', 'U');
  return s;
}

namespace internal {
bool FileKeyCore::tryCreate(size_t year, size_t month, size_t day, size_t part, bool extra,
    FileKeyCore *result, const FailFrame &ff) {
  if (year >= yearBegin && year < yearEnd &&
      month >= monthBegin && month < monthEnd &&
      day >= dayBegin && day < dayEnd &&
      part >= partBegin && part < partEnd) {
    *result = createUnsafe(year, month, day, part, extra);
    return true;
  }
  return ff.failf(HERE, "One of (%o,%o,%o,%o) is out of range", year, month, day, part);
}

bool FileKeyCore::tryCreate(uint32_t raw, FileKeyCore *result, const FailFrame &ff) {
  // Overkill: disassemble and reassemble for the purpose of error checking.
  auto isLogged = (raw % extraWidth) != 0;
  raw /= extraWidth;
  auto part = (raw % partWidth) + partBegin;
  raw /= partWidth;
  auto day = (raw % dayWidth) + dayBegin;
  raw /= dayWidth;
  auto month = (raw % monthWidth) + monthBegin;
  raw /= monthWidth;
  auto year = raw + yearBegin;
  return tryCreate(year, month, day, part, isLogged, result, ff.nest(HERE));
}

ExpandedFileKey FileKeyCore::asExpandedFileKey() const {
  auto value = raw_;
  auto extra = (value % extraWidth) != 0;
  value /= extraWidth;

  auto part = (value % partWidth) + partBegin;
  value /= partWidth;

  auto day = (value % dayWidth) + dayBegin;
  value /= dayWidth;

  auto month = (value % monthWidth) + monthBegin;
  value /= monthWidth;

  auto year = value + yearBegin;
  return {year, month, day, part, extra};
}

void FileKeyCore::dump(std::ostream &s, char extraWhenFalse, char extraWhenTrue) const {
  asExpandedFileKey().dump(s, extraWhenFalse, extraWhenTrue);
}
}  // namespace internal

bool DateAndPartKey::tryCreate(size_t year, size_t month, size_t day, size_t part,
    DateAndPartKey *result, const FailFrame &ff) {
  return internal::FileKeyCore::tryCreate(year, month, day, part, false, &result->core_,
      ff.nest(HERE));
}

bool DateAndPartKey::tryCreate(std::chrono::system_clock::time_point p, DateAndPartKey *result,
    const FailFrame &ff) {
  auto tp = std::chrono::system_clock::to_time_t(p);
  struct tm tm{};
  if (localtime_r(&tp, &tm) != &tm) {
    return ff.failf(HERE, "localtime_r failed, errno=%o", errno);
  }
  size_t year = tm.tm_year + 1900;
  size_t month = tm.tm_mon + 1;
  size_t day = tm.tm_mday;
  size_t part = 0;
  return DateAndPartKey::tryCreate(year, month, day, part, result, ff.nest(HERE));
}

bool DateAndPartKey::tryBump(DateAndPartKey *result, const FailFrame &ff) const {
  auto efk = asExpandedFileKey();
  return DateAndPartKey::tryCreate(efk.year(), efk.month(), efk.day(), efk.part() + 1, result,
      ff.nest(HERE));
}

bool DateAndPartKey::thisOrNow(std::chrono::system_clock::time_point now,
    DateAndPartKey *result, const FailFrame &ff) const {
  DateAndPartKey dpBasedOnClock;
  if (!DateAndPartKey::tryCreate(now, &dpBasedOnClock, ff.nest(HERE))) {
    return false;
  }

  *result = std::max(*this, dpBasedOnClock);
  return true;
}

FileKey DateAndPartKey::asFileKey(bool logged) const {
  auto efk = asExpandedFileKey();
  return FileKey::createUnsafe(efk.year(), efk.month(), efk.day(), efk.part(), logged);
}

bool FileKey::tryCreate(size_t year, size_t month, size_t day, size_t part, bool isLogged,
    FileKey *result, const FailFrame &ff) {
  return internal::FileKeyCore::tryCreate(year, month, day, part, isLogged, &result->core_,
      ff.nest(HERE));
}

bool FileKey::tryCreate(uint32 raw, FileKey *result, const FailFrame &ff) {
  return internal::FileKeyCore::tryCreate(raw, &result->core_, ff.nest(HERE));
}

DateAndPartKey FileKey::asDateAndPartKey() const {
  auto efk = core_.asExpandedFileKey();
  FailRoot fr;
  DateAndPartKey result;
  if (!DateAndPartKey::tryCreate(efk.year(), efk.month(), efk.day(), efk.part(), &result,
      fr.nest(HERE))) {
    crash("Can't happen: %o", fr);
  }
  return result;
}

int Location::compare(const Location &other) const {
  int difference;
  difference = fileKey_.compare(other.fileKey_);
  if (difference != 0) return difference;
  difference = kosak::coding::compare(&position_, &other.position_);
  if (difference != 0) return difference;
  difference = kosak::coding::compare(&size_, &other.size_);
  return difference;
}

std::ostream &operator<<(std::ostream &s, const Location &o) {
  return streamf(s, "[%o, pos=%o, size=%o]", o.fileKey_, o.position_, o.size_);
}
}  // namespace z2kplus::backend::files
