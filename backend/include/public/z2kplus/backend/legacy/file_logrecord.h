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

#include <memory>
#include <ostream>
#include <string>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "z2kplus/backend/files/keys.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/reverse_index/types.h"
#include "z2kplus/backend/legacy/file_logrecord.h"
#include "z2kplus/backend/legacy/zephyrgram.h"

namespace z2kplus::backend::legacy {

class LogParser {
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::files::FileKey FileKey;
  typedef z2kplus::backend::files::PathMaster PathMaster;
  typedef z2kplus::backend::legacy::LogRecord LogRecord;
  typedef z2kplus::backend::legacy::Zephyrgram Zephyrgram;

public:
  LogParser() = delete;

  static bool tryParseLogText(std::string_view text, std::vector<LogRecord> *records,
      const FailFrame &ff);
  static bool tryParseLogRecord(std::string_view text, LogRecord *result,
      const FailFrame &ff);
};

}  // namespace z2kplus::backend::legacy
