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
//std::pair<std::optional<TaggedFileKey<true>>, std::optional<TaggedFileKey<false>>>
//    ExpandedFileKey::visit() const {
//  std::optional<TaggedFileKey<true>> logged;
//  std::optional<TaggedFileKey<false>> unlogged;
//
//  if (isLogged_) {
//    logged = TaggedFileKey<true>(compress());
//  } else {
//    unlogged = TaggedFileKey<false>(compress());
//  }
//
//  return std::make_pair(logged, unlogged);
//}

std::ostream &operator<<(std::ostream &s, const LogLocation &o) {
  streamf(s, "%o offset %o size %o", o.fileKey_, o.offset_, o.size_);
  return s;
}

namespace internal {
uint32_t timePointToRaw(std::chrono::system_clock::time_point timePoint, bool isLogged) {
  std::time_t tt = std::chrono::system_clock::to_time_t(timePoint);
  struct tm tm = {};
  (void)gmtime_r(&tt, &tm);
  auto temp = FileKey<FileKeyKind::Either>::createUnsafe(tm.tm_year, tm.tm_mon + 1, tm.tm_mday, isLogged);
  return temp.raw();
}
}  // namespace internal
}  // namespace z2kplus::backend::files
