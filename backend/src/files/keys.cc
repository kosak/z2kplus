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
std::ostream &operator<<(std::ostream &s, const CompressedFileKey &o) {
  ExpandedFileKey efk(o);
  streamf(s, "%o (%o)", o.raw_, efk);
  return s;
}

bool ExpandedFileKey::tryCreate(uint32_t year, uint32_t month, uint32_t day, bool logged,
    ExpandedFileKey *result, const FailFrame &ff) {
  if (year > 2100 || month > 12 || day > 31) {
    return ff.failf(HERE, "At least one of (%o,%o,%o) is suspicious", year, month, day);
  }
  *result = createUnsafe(year, month, day, logged);
  return true;
}

ExpandedFileKey::ExpandedFileKey(CompressedFileKey cfk) {
  auto raw = cfk.raw();
  isLogged_ = (raw % 10) != 0;
  raw /= 10;

  day_ = raw % 100;
  raw /= 100;

  month_ = raw % 100;
  raw /= 100;

  year_ = raw;
}

std::pair<std::optional<TaggedFileKey<true>>, std::optional<TaggedFileKey<false>>>
    ExpandedFileKey::visit() const {
  std::optional<TaggedFileKey<true>> logged;
  std::optional<TaggedFileKey<false>> unlogged;

  if (isLogged_) {
    logged = TaggedFileKey<true>(compress());
  } else {
    unlogged = TaggedFileKey<false>(compress());
  }

  return std::make_pair(logged, unlogged);
}

std::ostream &operator<<(std::ostream &s, const ExpandedFileKey &o) {
  // yyyymmdd.{logged,unlogged}
  // Plenty of space
  char buffer[64];
  snprintf(buffer, STATIC_ARRAYSIZE(buffer), "%04u%02u%02u.%s",
      o.year_, o.month_, o.day_, o.isLogged_ ? "logged" : "unlogged");
  s << buffer;
  return s;
}

std::ostream &operator<<(std::ostream &s, const LogLocation &o) {
  streamf(s, "%o:[%o-%o)", o.fileKey_, o.begin_, o.end_);
  return s;
}
}  // namespace z2kplus::backend::files
