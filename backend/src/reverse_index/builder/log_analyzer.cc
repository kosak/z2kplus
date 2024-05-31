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
using z2kplus::backend::files::CompressedFileKey;
using z2kplus::backend::files::ExpandedFileKey;
using z2kplus::backend::files::InterFileRange;
using z2kplus::backend::files::IntraFileRange;
namespace nsunix = kosak::coding::nsunix;

namespace z2kplus::backend::reverse_index::builder {
namespace {
struct MyComparer {
  bool operator()(const IntraFileRange &lhs, const IntraFileRange &rhs) const {
    auto lLogged = lhs.fileKey().isLogged();
    auto rLogged = rhs.fileKey().isLogged();
    if (lLogged != rLogged) {
      // returning the result of "less than", assuming we want all logged before all unlogged
      // lLogged = false, rLogged = true: return false
      // lLogged = true, rLogged = false: return true
      return lLogged;
    }

    // Otherwise we can rely on the natural ordering of the raw value.
    return lhs.fileKey().raw() < rhs.fileKey().raw();
  }
};
}  // namespace
LogAnalyzer::LogAnalyzer() = default;
LogAnalyzer::LogAnalyzer(std::vector<IntraFileRange<true>> sortedLoggedRanges,
    std::vector<IntraFileRange<false>> sortedUnloggedRanges) :
    sortedLoggedRanges_(std::move(sortedLoggedRanges)),
    sortedUnloggedRanges_(std::move(sortedUnloggedRanges)) {}
LogAnalyzer::LogAnalyzer(LogAnalyzer &&) noexcept = default;
LogAnalyzer &LogAnalyzer::operator=(LogAnalyzer &&) noexcept = default;
LogAnalyzer::~LogAnalyzer() = default;

template<bool IsLogged>
bool tryProcessFile(const InterFileRange<IsLogged> &universe, CompressedFileKey key,
    uint32_t begin, uint32_t end, std::vector<IntraFileRange<IsLogged>> *result,
    const FailFrame &ff) {
  InterFileRange <IsLogged> inter;
  if (!InterFileRange<IsLogged>::tryCreate(key, begin, key, end, &inter, ff.nest(HERE))) {
    return false;
  }
  auto intersection = universe.intersectWith(inter);
  if (intersection.empty()) {
    return true;
  }

  IntraFileRange<IsLogged> intra;
  if (!IntraFileRange<IsLogged>::tryCreate())



    result->emplace_back()
    intersection.begin()
    return true;
  }

  InterFileRange<IsLogged> myRange(key, 0, key, stat.st_size);

}

bool LogAnalyzer::tryAnalyze(const PathMaster &pm,
    const InterFileRange<true> &loggedRange,
    const InterFileRange<false> &unloggedRange,
    LogAnalyzer *result, const FailFrame &ff) {
  std::vector<IntraFileRange<true>> includedLoggedRanges;
  std::vector<IntraFileRange<false>> includedUnloggedRanges;
  auto cbKeys = [&pm, &loggedRange, &unloggedRange, &includedRanges](
      const CompressedFileKey &key, const FailFrame &f2) {
    auto filename = pm.getPlaintextPath(key);
    FileCloser fc;
    struct stat stat = {};
    if (!nsunix::tryOpen(filename, O_RDONLY, 0, &fc, f2.nest(HERE)) ||
        !nsunix::tryFstat(fc.get(), &stat, f2.nest(HERE))) {
      return false;
    }

    ExpandedFileKey efk(key);
    if (efk.isLogged()) {
      processNubbin1();
    } else {
      processNubbin2();
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
  return streamf(s, "sorted=%o\nunsorted=%o", o.sortedLoggedRanges_, o.sortedUnloggedRanges_);
}
}  // namespace z2kplus::backend::reverse_index::builder
