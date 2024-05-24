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
struct LogSplitterResult {
  typedef z2kplus::backend::files::FileKey FileKey;
  LogSplitterResult();
  LogSplitterResult(std::vector<std::string> loggedZgrams, std::vector<std::string> unloggedZgrams,
      std::string reactionsByZgramId, std::string reactionsByReaction, std::string zgramRevisions,
      std::string zgramRefersTo, std::string zmojis);
  DISALLOW_COPY_AND_ASSIGN(LogSplitterResult);
  DECLARE_MOVE_COPY_AND_ASSIGN(LogSplitterResult);
  ~LogSplitterResult();

  std::vector<std::string> loggedZgrams_;
  std::vector<std::string> unloggedZgrams_;
  std::string reactionsByZgramId_;
  std::string reactionsByReaction_;
  std::string zgramRevisions_;
  std::string zgramRefersTo_;
  std::string zmojis_;
};

class LogSplitter {
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::files::PathMaster PathMaster;
  typedef z2kplus::backend::files::IntraFileRange IntraFileRange;

public:
  static bool split(const PathMaster &pm, const std::vector<IntraFileRange> &ranges,
      size_t numShards, LogSplitterResult *result, const FailFrame &ff);
};
}  // namespace z2kplus::backend::reverse_index::builder
