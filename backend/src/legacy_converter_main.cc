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

#include <clocale>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/text/conversions.h"
#include "kosak/coding/memory/mapped_file.h"
#include "z2kplus/backend/coordinator/coordinator.h"
#include "z2kplus/backend/server/server.h"
#include "z2kplus/backend/files/keys.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/legacy/file_logrecord.h"
#include "z2kplus/backend/legacy/zephyrgram.h"
#include "z2kplus/backend/reverse_index/index/consolidated_index.h"
#include "z2kplus/backend/reverse_index/index/frozen_index.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::streamf;
using kosak::coding::memory::MappedFile;
using kosak::coding::text::tryParseDecimal;
using z2kplus::backend::coordinator::Coordinator;
using z2kplus::backend::files::ExpandedFileKey;
using z2kplus::backend::files::FileKey;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::legacy::LogParser;
using LegacyEmotionalReaction = z2kplus::backend::legacy::EmotionalReaction;
using LegacyLogRecord = z2kplus::backend::legacy::LogRecord;
using LegacyMetadata = z2kplus::backend::legacy::Metadata;
using LegacyPerUseridMetadataCore = z2kplus::backend::legacy::PerUseridMetadataCore;
using LegacyPerZgramMetadataCore = z2kplus::backend::legacy::PerZgramMetadataCore;
using LegacyRenderStyle = z2kplus::backend::legacy::RenderStyle;
using LegacyZephyrgram = z2kplus::backend::legacy::Zephyrgram;
using LegacyZgramCore = z2kplus::backend::legacy::ZgramCore;
using LegacyZgramId = z2kplus::backend::legacy::ZgramId;
using z2kplus::backend::reverse_index::index::ConsolidatedIndex;
using z2kplus::backend::reverse_index::index::FrozenIndex;
using z2kplus::backend::reverse_index::ZgramInfo;
using z2kplus::backend::server::Server;
using z2kplus::backend::shared::LogRecord;
using z2kplus::backend::shared::MetadataRecord;
using z2kplus::backend::shared::RenderStyle;
using z2kplus::backend::shared::Zephyrgram;
using z2kplus::backend::shared::ZgramCore;
using z2kplus::backend::shared::ZgramId;

namespace nsunix = kosak::coding::nsunix;
namespace userMetadata = z2kplus::backend::shared::userMetadata;
namespace zgMetadata = z2kplus::backend::shared::zgMetadata;

#define HERE KOSAK_CODING_HERE

// uh oh hack job

namespace {
class Converter {
public:
  static bool tryConvertDir(const std::string &srcDir, const std::string &destDir,
      const FailFrame &ff);

private:
  bool tryScanForModifies(const std::string &srcPath, const FailFrame &ff);
  bool tryConvertFile(const std::string &srcPath, const std::string &destPath, const FailFrame &ff);

  bool tryConvertLegacyZephyrgram(LegacyZephyrgram *src, std::vector<LogRecord> *dest,
      const FailFrame &ff);
  bool tryConvertLegacyMetadata(LegacyMetadata *src, std::vector<LogRecord> *dest,
      const FailFrame &ff);

  bool tryConvertPerZgramMetadata(const LegacyZgramId &zgId, const LegacyPerZgramMetadataCore *pzmdc,
      std::vector<LogRecord> *dest, const FailFrame &ff);

    bool tryConvertEdits(const ZgramId &zgramId, const LegacyPerZgramMetadataCore::edits_t &edits,
      std::vector<LogRecord> *result, const FailFrame &ff);

  std::set<ZgramId> modifiedZgrams_;
  std::map<ZgramId, ZgramCore> zgramCache_;
};


ZgramCore convertZgramCore(std::string instance, std::string body,
    LegacyRenderStyle legacyRenderStyle);
void convertPerUseridMetadata(const std::string &user, LegacyPerUseridMetadataCore *pumdc,
    std::vector<LogRecord> *dest);
void convertEmotionalReactions(const ZgramId &zgramId,
    const LegacyPerZgramMetadataCore::reactions_t &reactions, std::vector<LogRecord> *dest);
void convertHashtags(const ZgramId &zgramId,
    const LegacyPerZgramMetadataCore::hashtags_t &hashtags, std::vector<LogRecord> *dest);
void convertRefersTo(const ZgramId &zgramId,
    const LegacyPerZgramMetadataCore::refersTo_t &refersTo, std::vector<LogRecord> *dest);
void convertZmojis(const std::string &user, LegacyPerUseridMetadataCore::zmojis_t *zmojis,
    std::vector<LogRecord> *dest);

struct LegacyFileKey {
  static bool tryCreate(uint32_t year, uint32_t month, uint32_t day, uint32_t part, bool isLogged,
      LegacyFileKey *result, const FailFrame &ff);
  static bool tryParse(std::string_view name, LegacyFileKey *result, const FailFrame &ff);

  uint32_t year_ = 0;
  uint32_t month_ = 0;
  uint32_t day_ = 0;
  uint32_t part_ = 0;
  bool isLogged_ = false;
};
}  // namespace

int main(int argc, char **argv) {
  kosak::coding::internal::Logger::elidePrefix(__FILE__, 0);
  if (argc != 3) {
    streamf(std::cerr, "Usage: %o srcDir destDir\n", argv[0]);
    exit(1);
  }

  std::string srcDir(argv[1]);
  std::string destDir(argv[2]);
  FailRoot fr;
  if (!Converter::tryConvertDir(srcDir, destDir, fr.nest(HERE))) {
    std::cerr << fr << '\n';
    exit(1);
  }
}

namespace {
bool Converter::tryConvertDir(const std::string &srcDir, const std::string &destDir,
    const FailFrame &ff) {
  std::shared_ptr<PathMaster> pm;
  if (!PathMaster::tryCreate(destDir, &pm, ff.nest(HERE))) {
    return false;
  }

  typedef std::pair<std::string, FileKey> entry_t;

  std::vector<entry_t> entries;

  auto gather = [&entries](std::string_view srcPath, bool isDir, const FailFrame &f2) {
    if (isDir) {
      return true;
    }
    auto slashPos = srcPath.rfind('/');
    auto srcName = slashPos == std::string_view::npos ? srcPath : srcPath.substr(slashPos + 1);
    if (srcName == ".git") {
      return true;
    }
    LegacyFileKey lfk;
    FileKey fileKey;
    if (!LegacyFileKey::tryParse(srcName, &lfk, f2) ||
        !FileKey::tryCreate(lfk.year_, lfk.month_, lfk.day_, lfk.part_, lfk.isLogged_, &fileKey, f2)) {
      return false;
    }
    entries.emplace_back(std::string(srcPath), fileKey);
    return true;
  };
  if (!nsunix::tryEnumerateFilesAndDirsRecursively(srcDir, &gather, ff.nest(HERE))) {
    return false;
  }
  std::sort(entries.begin(), entries.end(), [](const entry_t &lhs, const entry_t &rhs) {
    return lhs.second.compare(rhs.second) < 0;
  });

  Converter converter;

  // Do a first pass over the data searching for zgrams that have had modifies. We will need to cache these
  // bodies when we do a second pass.
  auto lastYearMonth = FileKey().asExpandedFileKey();
  for (const auto &entry : entries) {
    auto efk = entry.second.asExpandedFileKey();
    if (efk.year() != lastYearMonth.year() || efk.month() != lastYearMonth.month()) {
      debug("Scanning %o for modifies", entry.first);
      lastYearMonth = efk;
    }
    if (!converter.tryScanForModifies(entry.first, ff.nest(HERE))) {
      return false;
    }
  }
  lastYearMonth = FileKey().asExpandedFileKey();
  for (const auto &entry : entries) {
    auto efk = entry.second.asExpandedFileKey();
    if (efk.year() != lastYearMonth.year() || efk.month() != lastYearMonth.month()) {
      debug("Converting %o", entry.first);
      lastYearMonth = efk;
    }
    auto destPath = pm->getPlaintextPath(entry.second);
    if (!converter.tryConvertFile(entry.first, destPath, ff.nest(HERE))) {
      return false;
    }
  }
  return true;
}

bool Converter::tryScanForModifies(const std::string &srcPath, const FailFrame &ff) {
  std::vector<LegacyLogRecord> legacySrc;
  {
    std::string text;
    if (!nsunix::tryReadAll(srcPath, &text, ff.nest(HERE)) ||
        !LogParser::tryParseLogText(text, &legacySrc, ff.nest(HERE))) {
      return false;
    }
  }

  struct visitor_t {
    void operator()(const LegacyZephyrgram &) const {
      // do nothing
    }

    void operator()(const LegacyMetadata &lmd) const {
      for (const auto &[zg, pzmdc]: lmd.pzg()) {
        if (!pzmdc.edits().empty()) {
          ZgramId zgramId(zg.id());
          owner_->modifiedZgrams_.insert(zgramId);
        }
      }
    }

    Converter *owner_ = nullptr;
  };

  visitor_t v{this};
  for (auto &src: legacySrc) {
    std::visit(v, src.payload());
  }
  return true;
}

bool Converter::tryConvertFile(const std::string &srcPath, const std::string &destPath,
    const FailFrame &ff) {
  if (!nsunix::tryEnsureBaseExists(destPath, 0755, ff.nest(HERE))) {
    return false;
  }
  std::vector<LegacyLogRecord> legacySrc;
  {
    std::string text;
    if (!nsunix::tryReadAll(srcPath, &text, ff.nest(HERE)) ||
        !LogParser::tryParseLogText(text, &legacySrc, ff.nest(HERE))) {
      return false;
    }
  }

  struct visitor_t {
    bool operator()(LegacyZephyrgram &lzg) {
      return owner_->tryConvertLegacyZephyrgram(&lzg, &dest_, ff2_.nest(HERE));
    }

    bool operator()(LegacyMetadata &lmd) {
      return owner_->tryConvertLegacyMetadata(&lmd, &dest_, ff2_.nest(HERE));
    }

    Converter *owner_ = nullptr;
    const FailFrame &ff2_;
    std::vector<LogRecord> dest_;
  };

  visitor_t v{this, ff.nest(HERE)};
  for (auto &src: legacySrc) {
    std::visit(v, src.payload());
  }
  std::string result;
  for (const auto &record: v.dest_) {
    using kosak::coding::tryAppendJson;
    if (!tryAppendJson(record, &result, ff.nest(HERE))) {
      return false;
    }
    result += '\n';
  }
  return nsunix::tryWriteAll(destPath, result, ff.nest(HERE));
}

bool Converter::tryConvertLegacyZephyrgram(LegacyZephyrgram *src, std::vector<LogRecord> *dest,
    const FailFrame &/*ff*/) {
  auto &lzgc = src->zgramCore();
  auto zgc = convertZgramCore(std::move(lzgc.instance()), std::move(lzgc.body()),
      lzgc.renderStyle());
  auto zgId = ZgramId(src->zgramId().id());
  if (modifiedZgrams_.find(zgId) != modifiedZgrams_.end()) {
    zgramCache_.try_emplace(zgId, zgc);
  }
  Zephyrgram zg(zgId, src->timesecs(), std::move(lzgc.sender()), std::move(lzgc.signature()),
      src->isLogged(), std::move(zgc));
  dest->push_back(LogRecord(std::move(zg)));
  return true;
}

bool Converter::tryConvertLegacyMetadata(LegacyMetadata *src, std::vector<LogRecord> *dest,
    const FailFrame &ff) {
  for (auto &[zg, pzmdc]: src->pzg()) {
    if (!tryConvertPerZgramMetadata(zg, &pzmdc, dest, ff.nest(HERE))) {
      return false;
    }
  }
  for (auto &[user, pumdc]: src->pu()) {
    convertPerUseridMetadata(user, &pumdc, dest);
  }
  return true;
}

ZgramCore convertZgramCore(std::string instance, std::string body,
    LegacyRenderStyle legacyRenderStyle) {
  RenderStyle renderStyle;
  switch (legacyRenderStyle) {
    case LegacyRenderStyle::Default: {
      renderStyle = RenderStyle::Default;
      break;
    }
    case LegacyRenderStyle::MarkDeepMathAjax: {
      renderStyle = RenderStyle::MarkDeepMathJax;
      break;
    }
    // Shouldn't be any instances of legacy monospace in the repo
    case LegacyRenderStyle::Monospace:
    default: {
      notreached;
    }
  }
  return {std::move(instance), std::move(body), renderStyle};
}

bool Converter::tryConvertPerZgramMetadata(const LegacyZgramId &zgId,
    const LegacyPerZgramMetadataCore *pzmdc, std::vector<LogRecord> *dest, const FailFrame &ff) {
  ZgramId zgramId(zgId.id());
  convertEmotionalReactions(zgramId, pzmdc->reactions(), dest);
  convertHashtags(zgramId, pzmdc->hashtags(), dest);
  convertRefersTo(zgramId, pzmdc->refersTo(), dest);
  // skip bookmarks, referredFrom, threads
  return tryConvertEdits(zgramId, pzmdc->edits(), dest, ff.nest(HERE));
  // skip pluspluses, watches
}

void convertPerUseridMetadata(const std::string &user, LegacyPerUseridMetadataCore *pumdc,
    std::vector<LogRecord> *dest) {
  convertZmojis(user, &pumdc->zmojis(), dest);
}

void convertEmotionalReactions(const ZgramId &zgramId,
    const LegacyPerZgramMetadataCore::reactions_t &reactions, std::vector<LogRecord> *dest) {
  const char *likeReaction = "👍";
  const char *dislikeReaction = "👎";
  for (const auto &[creator, r]: reactions) {
    if (r == LegacyEmotionalReaction::Like || r == LegacyEmotionalReaction::None) {
      // Cancel any previous dislike
      zgMetadata::Reaction reaction(zgramId, dislikeReaction, creator, false);
      dest->push_back(LogRecord(MetadataRecord(std::move(reaction))));
    }
    if (r == LegacyEmotionalReaction::Dislike || r == LegacyEmotionalReaction::None) {
      // Cancel any previous like
      zgMetadata::Reaction reaction(zgramId, likeReaction, creator, false);
      dest->push_back(LogRecord(MetadataRecord(std::move(reaction))));
    }
    if (r == LegacyEmotionalReaction::Like) {
      // Add a like
      zgMetadata::Reaction reaction(zgramId, likeReaction, creator, true);
      dest->push_back(LogRecord(MetadataRecord(std::move(reaction))));
    }
    if (r == LegacyEmotionalReaction::Dislike) {
      // Cancel any previous like
      zgMetadata::Reaction reaction(zgramId, dislikeReaction, creator, true);
      dest->push_back(LogRecord(MetadataRecord(std::move(reaction))));
    }
  }
}

void convertHashtags(const ZgramId &zgramId,
    const LegacyPerZgramMetadataCore::hashtags_t &hashtags, std::vector<LogRecord> *dest) {
  for (const auto &[tag, inner]: hashtags) {
    for (const auto &[creator, enable]: inner) {
      zgMetadata::Reaction reaction(zgramId, tag, creator, enable);
      dest->push_back(LogRecord(MetadataRecord(std::move(reaction))));
    }
  }
}

void convertRefersTo(const ZgramId &zgramId,
    const LegacyPerZgramMetadataCore::refersTo_t &refersTo, std::vector<LogRecord> *dest) {
  for (const auto &[target, valid]: refersTo) {
    ZgramId newTarget(target.id());
    zgMetadata::ZgramRefersTo rt(zgramId, newTarget, valid);
    dest->push_back(LogRecord(MetadataRecord(std::move(rt))));
  }
}


bool Converter::tryConvertEdits(const ZgramId &zgramId,
    const LegacyPerZgramMetadataCore::edits_t &edits, std::vector<LogRecord> *result,
    const FailFrame &ff) {
  auto ip = zgramCache_.find(zgramId);
  if (ip == zgramCache_.end()) {
    return ff.failf(HERE, "Couldn't find %o in cache", zgramId);
  }

  const auto &original = ip->second;

  auto currentBody = original.body();

  for (auto &[ignore, sedit]: edits) {
    auto splitPos = sedit.find('\001');
    if (splitPos == std::string::npos) {
      return ff.failf(HERE, "Expected format string to have special \\001 split character, got %o",
          sedit);
    }
    std::string_view src(sedit.data(), splitPos);
    auto dest = sedit.substr(splitPos + 1);
    std::regex re;
    try {
      re = std::regex(src.data(), src.size());
    } catch (...) {
      warn("In zgram %o, regex %o is problematic", zgramId, src);
      continue;
    }
    currentBody = std::regex_replace(currentBody, re, dest);
    ZgramCore zgc(original.instance(), currentBody, original.renderStyle());
    zgMetadata::ZgramRevision revision(zgramId, std::move(zgc));
    result->push_back(LogRecord(MetadataRecord(std::move(revision))));
  }
  return true;
}

void convertZmojis(const std::string &user, LegacyPerUseridMetadataCore::zmojis_t *zmojis,
    std::vector<LogRecord> *dest) {
  for (auto &[unit, zmoji]: *zmojis) {
    userMetadata::Zmojis zmojiRecord(user, std::move(zmoji));
    dest->push_back(LogRecord(MetadataRecord(std::move(zmojiRecord))));
  }
}

bool LegacyFileKey::tryCreate(uint32_t year, uint32_t month, uint32_t day, uint32_t part,
    bool isLogged, LegacyFileKey *result, const FailFrame &/*ff*/) {
  result->year_ = year;
  result->month_ = month;
  result->day_ = day;
  result->part_ = part;
  result->isLogged_ = isLogged;
  return true;
}

bool LegacyFileKey::tryParse(std::string_view name, LegacyFileKey *result, const FailFrame &ff) {
  static std::regex regex(R"delim(plaintext.(\d\d\d\d)(\d\d)(\d\d)p(\d\d\d\d)(P|T))delim",
      std::regex_constants::optimize);
  std::cmatch match;
  if (!regex_match(name.begin(), name.end(), match, regex)) {
    return ff.failf(HERE, "%o did not match regex", name);
  }

  const auto &cm0 = match[1];
  const auto &cm1 = match[2];
  const auto &cm2 = match[3];
  const auto &cm3 = match[4];

  uint32 year, month, day, part;
  if (!tryParseDecimal(std::string_view(cm0.first, cm0.second - cm0.first), &year, nullptr, ff.nest(HERE)) ||
      !tryParseDecimal(std::string_view(cm1.first, cm1.second - cm1.first), &month, nullptr, ff.nest(HERE)) ||
      !tryParseDecimal(std::string_view(cm2.first, cm2.second - cm2.first), &day, nullptr, ff.nest(HERE)) ||
      !tryParseDecimal(std::string_view(cm3.first, cm3.second - cm3.first), &part, nullptr, ff.nest(HERE))) {
    return false;
  }

  bool isLogged = *match[5].first == 'P';
  return LegacyFileKey::tryCreate(year, month, day, part, isLogged, result, ff.nest(HERE));
}
}  // namespace
