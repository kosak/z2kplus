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

#include <ostream>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/reverse_index/builder/common.h"

namespace z2kplus::backend::reverse_index::builder {
class LogAnalyzer {
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::files::FileKeyKind FileKeyKind;
  typedef z2kplus::backend::files::PathMaster PathMaster;

  template<FileKeyKind Kind>
  using InterFileRange = z2kplus::backend::files::InterFileRange<Kind>;
  template<FileKeyKind Kind>
  using IntraFileRange = z2kplus::backend::files::IntraFileRange<Kind>;

public:
  static bool tryAnalyze(const PathMaster &pm,
      const InterFileRange<FileKeyKind::Logged> &loggedRange,
      const InterFileRange<FileKeyKind::Unlogged> &unloggedRange,
      LogAnalyzer *result, const FailFrame &ff);

  LogAnalyzer();
  DISALLOW_COPY_AND_ASSIGN(LogAnalyzer);
  DECLARE_MOVE_COPY_AND_ASSIGN(LogAnalyzer);
  ~LogAnalyzer();

  const auto &sortedLoggedRanges() const { return sortedLoggedRanges_; }
  const auto &sortedUnloggedRanges() const { return sortedUnloggedRanges_; }

private:
  explicit LogAnalyzer(std::vector<IntraFileRange<FileKeyKind::Logged>> sortedLoggedRanges,
      std::vector<IntraFileRange<FileKeyKind::Unlogged>> sortedUnloggedRanges);

  std::vector<IntraFileRange<FileKeyKind::Logged>> sortedLoggedRanges_;
  std::vector<IntraFileRange<FileKeyKind::Unlogged>> sortedUnloggedRanges_;

  friend std::ostream &operator<<(std::ostream &s, const LogAnalyzer &o);
};
}  // namespace z2kplus::backend::reverse_index::builder
