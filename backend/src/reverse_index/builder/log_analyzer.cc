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

#include "z2kplus/backend/reverse_index/builder/log_analyzer.h"

#include "kosak/coding/coding.h"
#include "kosak/coding/unix.h"
#include "z2kplus/backend/files/keys.h"

#define HERE KOSAK_CODING_HERE

using kosak::coding::streamf;
using kosak::coding::nsunix::FileCloser;
using z2kplus::backend::files::FileKey;
using z2kplus::backend::files::IntraFileRange;
namespace nsunix = kosak::coding::nsunix;

namespace z2kplus::backend::reverse_index::builder {
LogAnalyzer::LogAnalyzer() = default;
LogAnalyzer::LogAnalyzer(std::vector<IntraFileRange> includedRanges) :
    includedRanges_(std::move(includedRanges)) {}
LogAnalyzer::LogAnalyzer(LogAnalyzer &&) noexcept = default;
LogAnalyzer &LogAnalyzer::operator=(LogAnalyzer &&) noexcept = default;
LogAnalyzer::~LogAnalyzer() = default;

bool LogAnalyzer::tryAnalyze(const PathMaster &pm,
    const InterFileRange &loggedRange,
    const InterFileRange &unloggedRange,
    LogAnalyzer *result, const FailFrame &ff) {
  std::vector<IntraFileRange> includedRanges;
  auto cbKeys = [&pm, &loggedRange, &unloggedRange, &includedRanges](const FileKey &key, const FailFrame &f2) {
    auto filename = pm.getPlaintextPath(key);
    FileCloser fc;
    struct stat stat = {};
    if (!nsunix::tryOpen(filename, O_RDONLY, 0, &fc, f2.nest(HERE)) ||
        !nsunix::tryFstat(fc.get(), &stat, f2.nest(HERE))) {
      return false;
    }

    InterFileRange myRange(key, 0, key, stat.st_size);

    const auto *which = key.isLogged() ? &loggedRange : &unloggedRange;
    auto intersection = which->intersectWith(myRange);

    if (!intersection.empty()) {
      includedRanges.emplace_back(key,
          intersection.begin().position(),
          intersection.end().position());
    }
    return true;
  };
  if (!pm.tryGetPlaintexts(&cbKeys, ff.nest(HERE))) {
    return false;
  }
  // To make things readable
  std::sort(includedRanges.begin(), includedRanges.end(), MyComparer());
  *result = LogAnalyzer(std::move(includedRanges));
  warn("Created a LogAnalyzer: %o", *result);
  return true;
}

std::ostream &operator<<(std::ostream &s, const LogAnalyzer &o) {
  return streamf(s, "includedRanges=%o", o.includedRanges);
}
}  // namespace z2kplus::backend::reverse_index::builder
