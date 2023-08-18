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

#define HERE KOSAK_CODING_HERE

using kosak::coding::streamf;

namespace z2kplus::backend::reverse_index::builder {
LogAnalyzer::LogAnalyzer() = default;
LogAnalyzer::LogAnalyzer(std::vector<FileKey> includedKeys) :
    includedKeys_(std::move(includedKeys)) {}
LogAnalyzer::LogAnalyzer(LogAnalyzer &&) noexcept = default;
LogAnalyzer &LogAnalyzer::operator=(LogAnalyzer &&) noexcept = default;
LogAnalyzer::~LogAnalyzer() = default;

bool LogAnalyzer::tryAnalyze(const PathMaster &pm,
    std::optional<FileKey> loggedBegin, std::optional<FileKey> loggedEnd,
    std::optional<FileKey> unloggedBegin, std::optional<FileKey> unloggedEnd,
    LogAnalyzer *result, const FailFrame &ff) {
  std::vector<FileKey> includedKeys;
  auto cbKeys = [&loggedBegin, &loggedEnd, &unloggedBegin, &unloggedEnd, &includedKeys](
      const FileKey &key, const FailFrame &f2) {
    auto efk = key.asExpandedFileKey();
    const std::optional<FileKey> *whichBegin, *whichEnd;

    if (efk.isLogged()) {
      whichBegin = &loggedBegin;
      whichEnd = &loggedEnd;
    } else {
      whichBegin = &unloggedBegin;
      whichEnd = &unloggedEnd;
    }

    if ((!whichBegin->has_value() || key >= *whichBegin) &&
        (!whichEnd->has_value() || key < *whichEnd)) {
      includedKeys.push_back(key);
    }

    FileKey bumpedKey;
    if (!FileKey::tryCreate(efk.year(), efk.month(), efk.day(), efk.part() + 1, efk.isLogged(),
        &bumpedKey, f2.nest(HERE))) {
      return false;
    }
    return true;
  };
  if (!pm.tryGetPlaintexts(&cbKeys, ff.nest(HERE))) {
    return false;
  }
  std::sort(includedKeys.begin(), includedKeys.end());
  *result = LogAnalyzer(std::move(includedKeys));
  warn("Created a LogAnalyzer: %o", *result);
  return true;
}

std::ostream &operator<<(std::ostream &s, const LogAnalyzer &o) {
  return streamf(s, "includedKeys=%o", o.includedKeys_);
}
}  // namespace z2kplus::backend::reverse_index::builder
