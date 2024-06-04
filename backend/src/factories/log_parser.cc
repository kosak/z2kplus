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

#include "z2kplus/backend/factories/log_parser.h"

#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/delegate.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/memory/mapped_file.h"
#include "kosak/coding/myjson.h"
#include "kosak/coding/text/misc.h"
#include "z2kplus/backend/shared/zephyrgram.h"

namespace z2kplus::backend::factories {
using kosak::coding::Delegate;
using kosak::coding::FailFrame;
using kosak::coding::ParseContext;
using kosak::coding::memory::MappedFile;
using kosak::coding::text::Splitter;
using z2kplus::backend::files::FileKey;
using z2kplus::backend::files::FileKeyKind;
using z2kplus::backend::files::IntraFileRange;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::shared::LogRecord;
using z2kplus::backend::shared::ZgramCore;

#define HERE KOSAK_CODING_HERE

bool LogParser::tryParseLogFile(const PathMaster &pm, FileKey<FileKeyKind::Either> fileKey,
    std::vector<logRecordAndLocation_t> *logRecordsAndLocations, const FailFrame &ff) {
  auto fileName = pm.getPlaintextPath(fileKey);
  MappedFile<char> mf;
  if (!mf.tryMap(fileName, false, ff.nest(HERE)) ||
      !tryParseLogRecords(std::string_view(mf.get(), mf.byteSize()), fileKey,
          logRecordsAndLocations, ff.nest(HERE))) {
    return false;
  }
  return true;
}

bool LogParser::tryParseLogRecords(std::string_view text, FileKey<FileKeyKind::Either> fileKey,
    std::vector<logRecordAndLocation_t> *logRecordsAndLocations,
    const FailFrame &ff) {
  auto splitter = Splitter::ofRecords(text, '\n');
  size_t nextIndex = 0;
  size_t offset = 0;
  std::string_view line;
  while (splitter.moveNext(&line)) {
    LogRecord logRecord;
    if (!tryParseLogRecord(line, &logRecord, ff.nest(HERE))) {
      return ff.failf(HERE, "...at record %o (offset %o, size %o)", nextIndex, offset, line.size());
    }
    LogLocation location(fileKey, offset, offset + line.size());
    logRecordsAndLocations->emplace_back(std::move(logRecord), location);
    ++nextIndex;
    offset += line.size() + 1;
  }
  return true;
}

bool LogParser::tryParseLogRecord(std::string_view text, LogRecord *result, const FailFrame &ff) {
  using kosak::coding::tryParseJson;
  ParseContext ctx(text);
  return tryParseJson(&ctx, result, ff.nest(HERE));
}
}  // namespace z2kplus::backend::factories
