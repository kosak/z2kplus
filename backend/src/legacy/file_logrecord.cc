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

#include "z2kplus/backend/legacy/file_logrecord.h"

#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/myjson.h"
#include "z2kplus/backend/files/keys.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/util/misc.h"
#include "z2kplus/backend/legacy/zephyrgram.h"

namespace z2kplus::backend::legacy {

using z2kplus::backend::files::PathMaster;
using z2kplus::backend::legacy::LogRecord;
using z2kplus::backend::legacy::Zephyrgram;
using z2kplus::backend::legacy::ZgramCore;
using kosak::coding::FailFrame;
using kosak::coding::ParseContext;

#define HERE KOSAK_CODING_HERE

namespace {
bool trySplitRecords(std::string_view records, std::vector<std::pair<std::string_view, size_t>> *result,
  const FailFrame &ff);
}  // namespace

bool LogParser::tryParseLogText(std::string_view logText, std::vector<LogRecord> *records,
    const FailFrame &ff) {
  std::vector<std::pair<std::string_view, size_t>> splits;
  if (!trySplitRecords(logText, &splits, ff.nest(HERE))) {
    return false;
  }

  for (size_t i = 0; i < splits.size(); ++i) {
    const auto &split = splits[i];
    const auto &text = split.first;
    LogRecord record;
    if (!tryParseLogRecord(text, &record, ff.nest(HERE))) {
      return ff.failf(HERE, "...at record %o", i);
    }
    records->emplace_back(std::move(record));
  }
  return true;
}

bool LogParser::tryParseLogRecord(std::string_view text, LogRecord *result, const FailFrame &ff) {
  using kosak::coding::tryParseJson;
  ParseContext ctx(text);
  if (!tryParseJson(&ctx, result, ff.nest(HERE))) {
    return false;
  }
  ctx.consumeWhitespace();
  auto residual = ctx.asStringView();
  if (!residual.empty()) {
    return ff.failf(HERE, "Excess text in record: %o", residual);
  }
  return true;
}

namespace {
bool trySplitRecords(std::string_view records,
    std::vector<std::pair<std::string_view, size_t>> *result,
    const FailFrame &ff) {
  size_t offset = 0;
  while (offset < records.size()) {
    auto tail = records.substr(offset);
    auto newline = tail.find('\n');
    if (newline == std::string_view::npos) {
      return ff.fail(HERE, "Trailing material without final newline!");
    }
    auto record = tail.substr(0, (size_t)newline);
    auto offsetToUse = offset;
    offset += (size_t)newline + 1;
    if (record.empty()) {
      continue;
    }
    result->push_back(make_pair(record, offsetToUse));
  }
  return true;
}
}  // namespace
}  // namespace z2kplus::backend::legacy
