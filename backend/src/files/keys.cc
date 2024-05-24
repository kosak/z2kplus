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
std::ostream &operator<<(std::ostream &s, const LogLocation &o) {
  streamf(s, "%o offset %o size %o", o.fileKey_, o.offset_, o.size_);
  return s;
}

namespace {
bool tryCheckRange(const char *what, size_t value, size_t begin, size_t end, const FailFrame &ff) {
  if ((value < begin) || (value >= end)) {
    return ff.failf(HERE, "{%o} not in range [%o,%o)", value, begin, end);
  }
  return true;
}
}  // namespace

namespace internal {
bool tryValidate(uint32_t year, uint32_t month, uint32_t day, bool isLogged, FileKeyKind kind,
    const kosak::coding::FailFrame &ff) {
  if (!tryCheckRange("year", year, 1970, 2100 + 1, ff.nest(HERE)) ||
      !tryCheckRange("month", month, 1, 12 + 1, ff.nest(HERE)) ||
      !tryCheckRange("day", day, 1, 31 + 1, ff.nest(HERE))) {
    return false;
  }
  auto allowLogged = kind != FileKeyKind::Unlogged;
  auto allowUnlogged = kind != FileKeyKind::Logged;
  if ((isLogged && !allowLogged) || (!isLogged && !allowUnlogged)) {
    return ff.failf(HERE, "isLogged is %o but kind is %o", isLogged, (int)kind);
  }
  return true;
}

bool tryCreateRawFromTimePoint(std::chrono::system_clock::time_point timePoint, bool isLogged,
    uint32_t *result, const FailFrame &ff) {
  std::time_t tt = std::chrono::system_clock::to_time_t(timePoint);
  struct tm tm = {};
  (void)gmtime_r(&tt, &tm);
  auto year = tm.tm_year + 1900;
  auto month = tm.tm_mon + 1;
  auto day = tm.tm_mday;
  auto kind = isLogged ? FileKeyKind::Logged : FileKeyKind::Unlogged;
  if (!tryValidate(year, month, day, isLogged, kind, ff.nest(HERE))) {
    return false;
  }
  *result = FileKey<FileKeyKind::Either>::createUnsafe(year, month, day, isLogged).raw();
  return true;
}
}  // namespace internal
}  // namespace z2kplus::backend::files
