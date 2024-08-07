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

using kosak::coding::FailFrame;
using kosak::coding::streamf;
using kosak::coding::nsunix::FileCloser;
using z2kplus::backend::files::FileKey;
using z2kplus::backend::files::FileKeyKind;
using z2kplus::backend::files::InterFileRange;
using z2kplus::backend::files::IntraFileRange;
namespace nsunix = kosak::coding::nsunix;

namespace z2kplus::backend::reverse_index::builder {
LogAnalyzer::LogAnalyzer() = default;
LogAnalyzer::LogAnalyzer(std::vector<IntraFileRange<FileKeyKind::Logged>> sortedLoggedRanges,
    std::vector<IntraFileRange<FileKeyKind::Unlogged>> sortedUnloggedRanges) :
    sortedLoggedRanges_(std::move(sortedLoggedRanges)),
    sortedUnloggedRanges_(std::move(sortedUnloggedRanges)) {}
LogAnalyzer::LogAnalyzer(LogAnalyzer &&) noexcept = default;
LogAnalyzer &LogAnalyzer::operator=(LogAnalyzer &&) noexcept = default;
LogAnalyzer::~LogAnalyzer() = default;

template<FileKeyKind Kind>
void processFile(const InterFileRange<Kind> &universe, FileKey<Kind> key,
    uint32_t begin, uint32_t end, std::vector<IntraFileRange<Kind>> *result) {
  InterFileRange<Kind> inter(key, begin, key, end);
  auto intersection = universe.intersectWith(inter);
  if (!intersection.empty()) {
    passert(intersection.begin().fileKey().raw() == intersection.end().fileKey().raw());
    result->emplace_back(key, intersection.begin().position(), intersection.end().position());
  }
}

bool LogAnalyzer::tryAnalyze(const PathMaster &pm,
    const InterFileRange<FileKeyKind::Logged> &loggedRange,
    const InterFileRange<FileKeyKind::Unlogged> &unloggedRange,
    LogAnalyzer *result, const FailFrame &ff) {
  std::vector<IntraFileRange<FileKeyKind::Logged>> includedLoggedRanges;
  std::vector<IntraFileRange<FileKeyKind::Unlogged>> includedUnloggedRanges;
  auto cbKeys = [&](FileKey<FileKeyKind::Either> key, const FailFrame &f2) {
    auto filename = pm.getPlaintextPath(key);
    FileCloser fc;
    struct stat stat = {};
    if (!nsunix::tryOpen(filename, O_RDONLY, 0, &fc, f2.nest(HERE)) ||
        !nsunix::tryFstat(fc.get(), &stat, f2.nest(HERE))) {
      return false;
    }

    auto [loggedKey, unloggedKey] = key.visit();
    if (loggedKey.has_value()) {
      processFile(loggedRange, *loggedKey, 0, stat.st_size, &includedLoggedRanges);
    } else {
      processFile(unloggedRange, *unloggedKey, 0, stat.st_size, &includedUnloggedRanges);
    }
    return true;
  };
  if (!pm.tryGetPlaintexts(&cbKeys, ff.nest(HERE))) {
    return false;
  }
  // To make things readable
  std::sort(includedLoggedRanges.begin(), includedLoggedRanges.end(),
      [](const auto &lhs, const auto &rhs) { return lhs.fileKey().raw() < rhs.fileKey().raw(); });
  std::sort(includedUnloggedRanges.begin(), includedUnloggedRanges.end(),
      [](const auto &lhs, const auto &rhs) { return lhs.fileKey().raw() < rhs.fileKey().raw(); });
  *result = LogAnalyzer(std::move(includedLoggedRanges), std::move(includedUnloggedRanges));
  warn("Created a LogAnalyzer: %o", *result);
  return true;
}

std::ostream &operator<<(std::ostream &s, const LogAnalyzer &o) {
  return streamf(s, "logged=%o\nunlogged=%o", o.sortedLoggedRanges_, o.sortedUnloggedRanges_);
}
}  // namespace z2kplus::backend::reverse_index::builder
