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

namespace z2kplus::backend::reverse_index::builder {
class IndexBuilder {
  typedef z2kplus::backend::files::PathMaster PathMaster;
  typedef z2kplus::backend::files::DateAndPartKey DateAndPartKey;
  typedef z2kplus::backend::files::FileKey FileKey;
  typedef kosak::coding::FailFrame FailFrame;

public:
  IndexBuilder() = delete;

  static bool tryClearScratchDirectory(const PathMaster &pm, const FailFrame &ff);
  static bool tryBuild(const PathMaster &pm,
      std::optional<FileKey> loggedBeginKey, std::optional<FileKey> loggedEndKey,
      std::optional<FileKey> unloggedBeginKey, std::optional<FileKey> unloggedEndKey,
      const FailFrame &ff);
};
}   // namespace z2kplus::backend::reverse_index::builder
