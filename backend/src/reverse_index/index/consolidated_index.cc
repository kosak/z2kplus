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

#include "z2kplus/backend/reverse_index/index/consolidated_index.h"

#include <algorithm>
#include <string_view>
#include <fcntl.h>
#include "kosak/coding/containers/slice.h"
#include "kosak/coding/map_utils.h"
#include "kosak/coding/memory/maybe_inlined_buffer.h"
#include "z2kplus/backend/factories/log_parser.h"
#include "z2kplus/backend/reverse_index/builder/log_analyzer.h"
#include "z2kplus/backend/shared/logging_policy.h"
#include "z2kplus/backend/shared/util.h"
#include "z2kplus/backend/util/frozen/frozen_string_pool.h"
#include "z2kplus/backend/util/frozen/frozen_vector.h"

namespace z2kplus::backend::reverse_index::index {

using kosak::coding::FailFrame;
using kosak::coding::containers::asSlice;
using kosak::coding::makeReservedVector;
using kosak::coding::maputils::tryFind;
using kosak::coding::memory::MaybeInlinedBuffer;
using kosak::coding::nsunix::FileCloser;
using kosak::coding::streamf;
using z2kplus::backend::factories::LogParser;
using z2kplus::backend::files::CompressedFileKey;
using z2kplus::backend::files::ExpandedFileKey;
using z2kplus::backend::files::FilePosition;
using z2kplus::backend::files::InterFileRange;
using z2kplus::backend::files::IntraFileRange;
using z2kplus::backend::files::LogLocation;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::files::TaggedFileKey;
using z2kplus::backend::util::frozen::FrozenStringPool;
using z2kplus::backend::util::frozen::frozenStringRef_t;
using z2kplus::backend::reverse_index::builder::LogAnalyzer;
using z2kplus::backend::reverse_index::index::FrozenIndex;
using z2kplus::backend::reverse_index::metadata::DynamicMetadata;
using z2kplus::backend::reverse_index::metadata::FrozenMetadata;
using z2kplus::backend::shared::getZgramId;
using z2kplus::backend::shared::LogRecord;
using z2kplus::backend::shared::LoggingPolicy;
using z2kplus::backend::shared::MetadataRecord;
using z2kplus::backend::shared::PlusPlusScanner;
using z2kplus::backend::shared::RenderStyle;
using z2kplus::backend::shared::Zephyrgram;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::shared::ZgramCore;
using z2kplus::backend::util::automaton::DFANode;
using z2kplus::backend::util::automaton::FiniteAutomaton;
using z2kplus::backend::util::frozen::frozenStringRef_t;
using z2kplus::backend::util::frozen::FrozenVector;

#define HERE KOSAK_CODING_HERE

namespace magicConstants = z2kplus::backend::shared::magicConstants;
namespace nsunix = kosak::coding::nsunix;
namespace userMetadata = z2kplus::backend::shared::userMetadata;
namespace zgMetadata = z2kplus::backend::shared::zgMetadata;

namespace {
bool tryCreateOrAppendToLogFile(const PathMaster &pm, CompressedFileKey fileKey, uint32_t offset,
    FileCloser *fc, const FailFrame &ff);

bool tryAppendAndFlushHelper(std::string_view buffer, internal::DynamicFileState *state,
    const FailFrame &ff);

bool tryReadAllDynamicFiles(const PathMaster &pm,
    const std::vector<IntraFileRange<true>> &loggedKeys,
    const std::vector<IntraFileRange<false>> &unloggedKeys,
    std::vector<DynamicIndex::logRecordAndLocation_t> *result, const FailFrame &ff);

template<bool IsLogged>
FilePosition<IsLogged> calcStart(const std::vector<IntraFileRange<IsLogged>> &ranges,
    std::chrono::system_clock::time_point now) {
  ExpandedFileKey efk(now, IsLogged);
  TaggedFileKey<IsLogged> tfk(efk.compress());
  FilePosition<IsLogged> proposedStart(tfk, 0);

  if (!ranges.empty()) {
    FilePosition<IsLogged> lastUsed(ranges.back().fileKey(), ranges.back().end());
    if (proposedStart < lastUsed) {
      proposedStart = lastUsed;
    }
  }
  return proposedStart;
}
}  // namespace

bool ConsolidatedIndex::tryCreate(std::shared_ptr<PathMaster> pm,
    std::chrono::system_clock::time_point now, ConsolidatedIndex *result, const FailFrame &ff) {
  // These are files not included in the index (i.e. written recently)
  MappedFile<FrozenIndex> frozenIndex;
  if (!frozenIndex.tryMap(pm->getIndexPath(), false, ff.nest(HERE))) {
    return false;
  }
  // Populate the dynamic index with all files newer than those in the frozen index.
  LogAnalyzer analyzer;

  InterFileRange<true> loggedRange(frozenIndex.get()->loggedEnd(), {});
  InterFileRange<false> unloggedRange(frozenIndex.get()->unloggedEnd(), {});
  if (!LogAnalyzer::tryAnalyze(*pm, loggedRange, unloggedRange, &analyzer, ff.nest(HERE))) {
    return false;
  }

  auto loggedStart = calcStart(analyzer.sortedLoggedRanges(), now);
  auto unloggedStart = calcStart(analyzer.sortedUnloggedRanges(), now);

  warn("logged=%o, unlogged%=o, loggedStart=%o, unloggedStart=%o",
      analyzer.sortedLoggedRanges(), analyzer.sortedUnloggedRanges(), loggedStart, unloggedStart);
  ConsolidatedIndex ci;
  std::vector<DynamicIndex::logRecordAndLocation_t> records;
  if (!tryCreate(std::move(pm), loggedStart, unloggedStart, std::move(frozenIndex), &ci, ff.nest(HERE)) ||
      !tryReadAllDynamicFiles(ci.pm(), analyzer.sortedLoggedRanges(), analyzer.sortedUnloggedRanges(),
          &records, ff.nest(HERE)) ||
      !ci.tryAddForBootstrap(records, ff.nest(HERE))) {
    return false;
  }

  *result = std::move(ci);
  return true;
}

bool ConsolidatedIndex::tryCreate(std::shared_ptr<PathMaster> pm,
    const FilePosition<true> &loggedStart, const FilePosition<false> &unloggedStart,
    MappedFile<FrozenIndex> frozenIndex, ConsolidatedIndex *result,
    const FailFrame &ff) {
  internal::DynamicFileState loggedState;
  internal::DynamicFileState unloggedState;
  if (!internal::DynamicFileState::tryCreate(*pm, loggedStart.fileKey().key(), loggedStart.position(),
      &loggedState, ff.nest(HERE)) ||
      !internal::DynamicFileState::tryCreate(*pm, unloggedStart.fileKey().key(), unloggedStart.position(),
          &unloggedState, ff.nest(HERE))) {
    return false;
  }

  *result = ConsolidatedIndex(std::move(pm), std::move(frozenIndex),
      std::move(loggedState), std::move(unloggedState));
  return true;
}

ConsolidatedIndex::ConsolidatedIndex() = default;

ConsolidatedIndex::ConsolidatedIndex(std::shared_ptr<PathMaster> &&pm,
    MappedFile<FrozenIndex> &&frozenIndex, internal::DynamicFileState &&loggedState,
    internal::DynamicFileState &&unloggedState) :
    pm_(std::move(pm)),
    frozenIndex_(std::move(frozenIndex)),
    loggedState_(std::move(loggedState)), unloggedState_(std::move(unloggedState)),
    zgramCache_(magicConstants::zgramCacheSize) {
}

ConsolidatedIndex::ConsolidatedIndex(ConsolidatedIndex &&other) noexcept = default;

ConsolidatedIndex &ConsolidatedIndex::operator=(ConsolidatedIndex &&other) noexcept = default;

ConsolidatedIndex::~ConsolidatedIndex() = default;

void ConsolidatedIndex::findMatching(const FiniteAutomaton &dfa,
    const kosak::coding::Delegate<void, const wordOff_t *, const wordOff_t *> &callback) const {
  frozenIndex_.get()->trie().findMatching(dfa, callback);
  dynamicIndex_.trie().findMatching(dfa, callback);
}

class PlusPlusManager {
public:
  explicit PlusPlusManager(ConsolidatedIndex *ci);
  ~PlusPlusManager();

  bool tryAddZgrams(const std::vector<Zephyrgram> &zgrams, const FailFrame &ff);

  bool tryAddMetadataRecords(const std::vector<MetadataRecord> &mdrs, const FailFrame &ff);

  bool tryAddLogRecords(const std::vector<DynamicIndex::logRecordAndLocation_t> &logRecords,
      const FailFrame &ff);

  bool tryAddZgram(const Zephyrgram &zgram, const FailFrame &ff);

  bool tryAddMetadataRecord(const MetadataRecord &mdr, const FailFrame &ff);

  bool tryAddLogRecord(const DynamicIndex::logRecordAndLocation_t &logRecord, const FailFrame &ff);

  bool tryFinish(const FailFrame &ff);

  DynamicIndex::ppDeltaMap_t &deltaMap() { return deltaMap_; }

private:
  ConsolidatedIndex *ci_ = nullptr;
  PlusPlusScanner plusPlusScanner_;
  DynamicIndex::ppDeltaMap_t deltaMap_;
  std::vector<std::pair<ZgramId, LogLocation>> locators_;
  bool finished_ = false;
};

bool ConsolidatedIndex::tryAddZgrams(std::chrono::system_clock::time_point now, const Profile &profile,
    std::vector<ZgramCore> &&zgcs, ppDeltaMap_t *deltaMap, std::vector<Zephyrgram> *zgrams,
    const FailFrame &ff) {
  PlusPlusManager ppm(this);
  if (!tryAddZgramsHelper(now, profile, std::move(zgcs), zgrams, ff.nest(HERE)) ||
      !ppm.tryAddZgrams(*zgrams, ff.nest(HERE)) ||
      !ppm.tryFinish(ff.nest(HERE))) {
    return false;
  }

  dynamicIndex_.batchUpdatePlusPlusCounts(ppm.deltaMap());
  *deltaMap = std::move(ppm.deltaMap());
  return true;
}

bool ConsolidatedIndex::tryAddMetadata(std::vector<MetadataRecord> &&records, ppDeltaMap_t *deltaMap,
    std::vector<MetadataRecord> *movedRecords, const FailFrame &ff) {
  PlusPlusManager ppm(this);
  if (!ppm.tryAddMetadataRecords(records, ff.nest(HERE)) ||
      !tryAddMetadataHelper(std::move(records), movedRecords, ff.nest(HERE)) ||
      !ppm.tryFinish(ff.nest(HERE))) {
    return false;
  }

  dynamicIndex_.batchUpdatePlusPlusCounts(ppm.deltaMap());
  *deltaMap = std::move(ppm.deltaMap());
  return true;
}

bool ConsolidatedIndex::tryAddForBootstrap(const std::vector<logRecordAndLocation_t> &records,
    const FailFrame &ff) {
  PlusPlusManager ppm(this);
  if (!dynamicIndex_.tryAddLogRecords(frozenIndex(), records, ff.nest(HERE)) ||
      !ppm.tryAddLogRecords(records, ff.nest(HERE)) ||
      !ppm.tryFinish(ff.nest(HERE))) {
    return false;
  }

  dynamicIndex_.batchUpdatePlusPlusCounts(ppm.deltaMap());
  return true;
}

bool
ConsolidatedIndex::tryAddZgramsHelper(std::chrono::system_clock::time_point now, const Profile &profile,
    std::vector<ZgramCore> &&zgcs, std::vector<Zephyrgram> *zgrams, const FailFrame &ff) {
  if (zgcs.empty()) {
    return true;
  }

  // 1. Cook the records (which will endow the zgrams with ids and timestamps)
  // 2. Write the records to the file (which will determine their locations)
  // 3. Add the records to the index

  // Endow each zgram with an id and timesecs
  ZgramId nextZgramId(0);
  if (zgramInfoSize() > 0) {
    nextZgramId = getZgramInfo(zgramOff_t(zgramInfoSize() - 1)).zgramId().next();
  }

  uint64_t timesecs = std::chrono::duration_cast<std::chrono::seconds>(
      now.time_since_epoch()).count();

  auto cooked = makeReservedVector<std::pair<LogRecord, LogLocation>>(zgcs.size());
  std::string loggedBuffer;
  std::string unloggedBuffer;
  for (auto &zgc : zgcs) {
    auto isLogged = LoggingPolicy::isLogged(zgc);
    Zephyrgram zgram(nextZgramId, timesecs, profile.userId(), profile.signature(), isLogged,
        std::move(zgc));
    nextZgramId = nextZgramId.next();

    internal::DynamicFileState *fs;
    std::string *buf;
    if (isLogged) {
      fs = &loggedState_;
      buf = &loggedBuffer;
    } else {
      fs = &unloggedState_;
      buf = &unloggedBuffer;
    }

    LogRecord logRecord(std::move(zgram));
    auto startBufferSize = buf->size();
    if (!tryAppendJson(logRecord, buf, ff.nest(HERE))) {
      return false;
    }
    buf->push_back('\n');
    auto recordSize = buf->size() - startBufferSize;
    LogLocation location(fs->fileKey(), fs->fileSize() + startBufferSize, recordSize);

    cooked.emplace_back(std::move(logRecord), location);
  }

  if (!tryAppendAndFlush(loggedBuffer, unloggedBuffer, ff.nest(HERE)) ||
      !dynamicIndex_.tryAddLogRecords(frozenIndex(), cooked, ff.nest(HERE))) {
    return false;
  }

  // Move the Zephyrgrams back out of the cooked vector
  for (auto &items : cooked) {
    auto &lr = items.first;
    auto *zg = std::get_if<Zephyrgram>(&lr.payload());
    if (zg == nullptr) {
      crash("Impossible");
    }
    zgrams->push_back(std::move(*zg));
  }
  return true;
}

bool ConsolidatedIndex::tryAddMetadataHelper(std::vector<MetadataRecord> &&metadata,
    std::vector<MetadataRecord> *movedMetadata, const FailFrame &ff) {
  if (metadata.empty()) {
    movedMetadata->clear();
    return true;
  }

  std::string loggedBuffer;
  std::string unloggedBuffer;
  for (auto &mr : metadata) {
    bool isLogged;
    if (!tryDetermineLogged(mr, &isLogged, ff.nest(HERE))) {
      return false;
    }
    std::string *buf = isLogged ? &loggedBuffer : &unloggedBuffer;

    // "borrow" the metadata
    LogRecord logRecord(std::move(mr));
    if (!tryAppendJson(logRecord, buf, ff.nest(HERE))) {
      return false;
    }
    buf->push_back('\n');
    // return the "borrowed" metadata
    mr = std::move(std::get<MetadataRecord>(logRecord.payload()));
  }

  if (!tryAppendAndFlush(loggedBuffer, unloggedBuffer, ff.nest(HERE)) ||
      !dynamicIndex_.tryAddMetadata(frozenIndex(), asSlice(metadata), ff.nest(HERE))) {
    return false;
  }
  *movedMetadata = std::move(metadata);
  return true;
}

PlusPlusManager::PlusPlusManager(ConsolidatedIndex *ci) : ci_(ci) {}
PlusPlusManager::~PlusPlusManager() {
  if (!finished_) {
    std::cerr << "Did you forget to call PlusPlusManager::tryFinish?\n";
  }
}

bool PlusPlusManager::tryAddZgrams(const std::vector<Zephyrgram> &zgrams, const FailFrame &ff) {
  for (const auto &zgram : zgrams) {
    if (!tryAddZgram(zgram, ff.nest(HERE))) {
      return false;
    }
  }
  return true;
}

bool PlusPlusManager::tryAddMetadataRecords(const std::vector<MetadataRecord> &mdrs,
    const FailFrame &ff) {
  for (const auto &mdr : mdrs) {
    if (!tryAddMetadataRecord(mdr, ff.nest(HERE))) {
      return false;
    }
  }
  return true;
}

bool
PlusPlusManager::tryAddLogRecords(const std::vector<DynamicIndex::logRecordAndLocation_t> &records,
    const FailFrame &ff) {
  for (const auto &record : records) {
    if (!tryAddLogRecord(record, ff.nest(HERE))) {
      return false;
    }
  }
  return true;
}

bool PlusPlusManager::tryAddLogRecord(const DynamicIndex::logRecordAndLocation_t &logRecord,
    const FailFrame &ff) {
  struct visitor_t {
    void operator()(const Zephyrgram &o) {
      zgram = &o;
    }
    void operator()(const MetadataRecord &o) {
      mdr = &o;
    }
    const Zephyrgram *zgram = nullptr;
    const MetadataRecord *mdr = nullptr;
  };

  visitor_t v;
  std::visit(v, logRecord.first.payload());
  if (v.zgram != nullptr) {
    return tryAddZgram(*v.zgram, ff.nest(HERE));
  }

  return tryAddMetadataRecord(*v.mdr, ff.nest(HERE));
}

bool PlusPlusManager::tryAddZgram(const Zephyrgram &zgram, const FailFrame &ff) {
  auto &inner = deltaMap_[zgram.zgramId()];
  plusPlusScanner_.scan(zgram.zgramCore().body(), 1, &inner);
  return true;
}

bool PlusPlusManager::tryAddMetadataRecord(const MetadataRecord &mdr, const FailFrame &ff) {
  // For each zgram revision, add in the new plusplus counts and go looking for the original or
  // latest revision so you can net out the old plusplus counts.

  const auto *zgr = std::get_if<shared::zgMetadata::ZgramRevision>(&mdr.payload());
  if (zgr == nullptr) {
    return true;
  }
  auto &inner = deltaMap_[zgr->zgramId()];

  // Add in the new ++ values
  plusPlusScanner_.scan(zgr->zgc().body(), 1, &inner);

  // If the zgram has already been modified, subtract out the old plusplus values from the
  // previous modification

  std::vector<zgMetadata::ZgramRevision> priorZgramRevs;
  ci_->getZgramRevsFor(zgr->zgramId(), &priorZgramRevs);
  if (!priorZgramRevs.empty()) {
    // Subtract out the old ++ values
    plusPlusScanner_.scan(priorZgramRevs.back().zgc().body(), -1, &inner);
    return true;
  }

  // No previous revisions. So we have to go looking in a different place, using deferred logic,
  // for the original zgram.
  zgramOff_t zgramOff;
  if (!ci_->tryFind(zgr->zgramId(), &zgramOff)) {
    return ff.failf(HERE, "Couldn't find zgramId %o", zgr->zgramId());
  }
  const auto &location = ci_->getZgramInfo(zgramOff).location();
  locators_.emplace_back(zgr->zgramId(), location);
  return true;
}

bool PlusPlusManager::tryFinish(const FailFrame &ff) {
  auto lookedUpZgrams = makeReservedVector<std::shared_ptr<const Zephyrgram>>(locators_.size());
  if (!ci_->zgramCache().tryLookupOrResolve(ci_->pm(), locators_, &lookedUpZgrams, ff.nest(HERE))) {
    return false;
  }

  for (const auto &zg : lookedUpZgrams) {
    auto &inner = deltaMap_[zg->zgramId()];
    plusPlusScanner_.scan(zg->zgramCore().body(), -1, &inner);
  }
  finished_ = true;
  return true;
}

bool ConsolidatedIndex::tryCheckpoint(std::chrono::system_clock::time_point now,
    FilePosition<true> *loggedPosition, FilePosition<false> *unloggedPosition, const FailFrame &/*ff*/) {
  *loggedPosition = FilePosition<true>(TaggedFileKey<true>(loggedState_.fileKey()), loggedState_.fileSize());
  *unloggedPosition = FilePosition<false>(TaggedFileKey<false>(unloggedState_.fileKey()), unloggedState_.fileSize());
  return true;
}

bool ConsolidatedIndex::tryFind(ZgramId id, zgramOff_t *result) const {
  auto lb = lowerBound(id);
  if (lb == zgramOff_t(zgramInfoSize()) || getZgramInfo(lb).zgramId() != id) {
    return false;
  }
  *result = lb;
  return true;
}

zgramOff_t ConsolidatedIndex::lowerBound(uint64_t timestamp) const {
  struct {
    bool operator()(const ZgramInfo &lhs, uint64_t rhs) const {
      return lhs.timesecs() < rhs;
    }

    bool operator()(uint64_t lhs, const ZgramInfo &rhs) const {
      return lhs < rhs.timesecs();
    }
  } idLess;

  const auto &fInfos = frozenIndex_.get()->zgramInfos();
  const auto fit = std::lower_bound(fInfos.begin(), fInfos.end(), timestamp, idLess);
  auto fdist = std::distance(fInfos.begin(), fit);
  if (fit != fInfos.end()) {
    return zgramOff_t(fdist);
  }

  const auto &dInfos = dynamicIndex_.zgramInfos();
  const auto dit = std::lower_bound(dInfos.begin(), dInfos.end(), timestamp, idLess);
  auto ddist = std::distance(dInfos.begin(), dit);
  return zgramOff_t(fdist + ddist);
}

zgramOff_t ConsolidatedIndex::lowerBound(ZgramId id) const {
  struct {
    bool operator()(const ZgramInfo &lhs, ZgramId rhs) const {
      return lhs.zgramId() < rhs;
    }

    bool operator()(ZgramId lhs, const ZgramInfo &rhs) const {
      return lhs < rhs.zgramId();
    }
  } idLess;

  const auto &fInfos = frozenIndex_.get()->zgramInfos();
  const auto fit = std::lower_bound(fInfos.begin(), fInfos.end(), id, idLess);
  auto fdist = std::distance(fInfos.begin(), fit);
  if (fit != fInfos.end()) {
    return zgramOff_t(fdist);
  }

  const auto &dInfos = dynamicIndex_.zgramInfos();
  const auto dit = std::lower_bound(dInfos.begin(), dInfos.end(), id, idLess);
  auto ddist = std::distance(dInfos.begin(), dit);
  return zgramOff_t(fdist + ddist);
}


const ZgramInfo &ConsolidatedIndex::getZgramInfo(zgramOff_t zgOff) const {
  auto index = zgOff.raw();
  const auto &fInfos = frozenIndex_.get()->zgramInfos();
  if (index < fInfos.size()) {
    return fInfos[index];
  }
  index -= fInfos.size();

  const auto &dInfos = dynamicIndex_.zgramInfos();
  passert(index < dInfos.size(), zgOff, index, fInfos.size(), dInfos.size());
  return dInfos[index];
}

const WordInfo &ConsolidatedIndex::getWordInfo(wordOff_t wordOff) const {
  auto index = wordOff.raw();
  const auto &fInfos = frozenIndex_.get()->wordInfos();
  if (index < fInfos.size()) {
    return fInfos[index];
  }
  index -= fInfos.size();

  const auto &dInfos = dynamicIndex_.wordInfos();
  passert(index < dInfos.size(), wordOff, fInfos.size(), dInfos.size());
  return dInfos[index];
}

ZgramId ConsolidatedIndex::zgramEnd() const {
  auto zgOff = zgramEndOff();
  if (zgOff.raw() == 0) {
    return ZgramId(0);
  }
  return getZgramInfo(zgOff.subtractRaw(1)).zgramId().next();
}

void ConsolidatedIndex::getMetadataFor(ZgramId zgramId, std::vector<MetadataRecord> *result) const {
  std::vector<zgMetadata::Reaction> reactions;
  std::vector<zgMetadata::ZgramRevision> zgRevisions;
  std::vector<zgMetadata::ZgramRefersTo> zgRefersTo;
  getReactionsFor(zgramId, &reactions);
  getZgramRevsFor(zgramId, &zgRevisions);
  getRefersToFor(zgramId, &zgRefersTo);
  for (auto &rx : reactions) {
    result->emplace_back(std::move(rx));
  }
  for (auto &rev : zgRevisions) {
    result->emplace_back(std::move(rev));
  }
  for (auto &rt : zgRefersTo) {
    result->emplace_back(std::move(rt));
  }
}

namespace {
void getReactions1(const FrozenStringPool &fsp, ZgramId zgramId, std::string_view reaction,
    const FrozenMetadata::reactions_t::mapped_type::mapped_type &fInner2,
    const DynamicMetadata::reactions_t::mapped_type::mapped_type &dInner2,
    std::vector<zgMetadata::Reaction> *result) {
  auto fp = fInner2.begin();
  auto dp = dInner2.begin();
  while (fp != fInner2.end() || dp != dInner2.end()) {
    int diff;
    if (dp == dInner2.end()) {
      diff = -1;
    } else if (fp == fInner2.end()) {
      diff = 1;
    } else {
      auto fCreator = fsp.toStringView(*fp);
      diff = fCreator.compare(dp->first);
    }

    std::string_view creator;
    bool value;
    if (diff <= 0) {
      creator = fsp.toStringView(*fp);
      // The fixed side has an implicit value=true
      value = true;
      ++fp;
    }
    if (diff >= 0) {
      creator = dp->first;
      value = dp->second;
      ++dp;
    }

    if (value) {
      result->emplace_back(zgramId, std::string(reaction), std::string(creator), true);
    }
  }
}
}

void ConsolidatedIndex::getReactionsFor(ZgramId zgramId, std::vector<zgMetadata::Reaction> *result) const {
  const auto &fsp = frozenIndex_.get()->stringPool();

  // Push frozen items not overridden by dynamic items
  typedef FrozenMetadata::reactions_t::mapped_type fInner_t;
  typedef DynamicMetadata::reactions_t::mapped_type dInner_t;

  fInner_t emptyFinner;
  dInner_t emptyDinner;

  const fInner_t *fInner;
  const dInner_t *dInner;
  if (!frozenIndex_.get()->metadata().reactions().tryFind(zgramId, &fInner)) {
    fInner = &emptyFinner;
  }
  if (!kosak::coding::maputils::tryFind(dynamicIndex_.metadata().reactions(), zgramId, &dInner)) {
    dInner = &emptyDinner;
  }

  auto fp = fInner->begin();
  auto dp = dInner->begin();

  typedef fInner_t::mapped_type fInner2_t;
  typedef dInner_t::mapped_type dInner2_t;
  dInner2_t emptyDinner2;
  fInner2_t emptyFinner2;
  while (fp != fInner->end() || dp != dInner->end()) {
    int diff;
    if (dp == dInner->end()) {
      diff = -1;
    } else if (fp == fInner->end()) {
      diff = 1;
    } else {
      auto fReaction = fsp.toStringView(fp->first);
      diff = fReaction.compare(dp->first);
    }

    std::string_view reaction;
    const auto *fInner2 = &emptyFinner2;
    const auto *dInner2 = &emptyDinner2;
    if (diff <= 0) {
      reaction = fsp.toStringView(fp->first);
      fInner2 = &fp->second;
      ++fp;
    }
    if (diff >= 0) {
      reaction = dp->first;
      dInner2 = &dp->second;
      ++dp;
    }

    getReactions1(fsp, zgramId, reaction, *fInner2, *dInner2, result);
  }
}

void ConsolidatedIndex::getZgramRevsFor(ZgramId zgramId, std::vector<zgMetadata::ZgramRevision> *result) const {
  const auto &fsp = frozenIndex_.get()->stringPool();

  // All items get sent. Frozen items first...
  {
    typedef FrozenMetadata::zgramRevisions_t::mapped_type fInner_t;
    const fInner_t *fInner;
    if (frozenIndex_.get()->metadata().zgramRevisions().tryFind(zgramId, &fInner)) {
      for (const auto &item : *fInner) {
        auto instanceRef = std::get<0>(item);
        auto bodyRef = std::get<1>(item);
        auto renderStyle = std::get<2>(item);
        auto instanceSv = fsp.toStringView(instanceRef);
        auto bodySv = fsp.toStringView(bodyRef);
        ZgramCore zgc(std::string(instanceSv), std::string(bodySv), (RenderStyle)renderStyle);
        result->emplace_back(zgramId, std::move(zgc));
      }
    }
  }
  // ... followed by dynamic items
  typedef DynamicMetadata::zgramRevisions_t::mapped_type dInner_t;
  const dInner_t *dInner;
  if (kosak::coding::maputils::tryFind(dynamicIndex_.metadata().zgramRevisions(), zgramId, &dInner)) {
    for (const auto &item : *dInner) {
      ZgramCore zgCopy(item);
      result->emplace_back(zgramId, std::move(zgCopy));
    }
  }
}

void ConsolidatedIndex::getRefersToFor(ZgramId zgramId, std::vector<zgMetadata::ZgramRefersTo> *result) const {
  // Push frozen items not overridden by dynamic items
  typedef FrozenMetadata::zgramRefersTo_t::mapped_type fInner_t;
  typedef DynamicMetadata::zgramRefersTo_t::mapped_type dInner_t;

  fInner_t emptyFinner;
  dInner_t emptyDinner;

  const fInner_t *fInner;
  const dInner_t *dInner;
  if (!frozenIndex_.get()->metadata().zgramRefersTo().tryFind(zgramId, &fInner)) {
    fInner = &emptyFinner;
  }
  if (!kosak::coding::maputils::tryFind(dynamicIndex_.metadata().zgramRefersTo(), zgramId, &dInner)) {
    dInner = &emptyDinner;
  }

  auto fp = fInner->begin();
  auto dp = dInner->begin();

  while (fp != fInner->end() || dp != dInner->end()) {
    int diff;
    if (dp == dInner->end()) {
      // Only in frozen
      diff = -1;
    } else if (fp == fInner->end()) {
      // Only in dynamic
      diff = 1;
    } else {
      // In both
      diff = 0;
    }

    ZgramId refersTo;
    bool value;
    if (diff <= 0) {
      refersTo = *fp;
      value = true;
      ++fp;
    }
    if (diff >= 0) {
      refersTo = dp->first;
      value = dp->second;
      ++dp;
    }

    if (!value) {
      continue;
    }
    result->emplace_back(zgramId, refersTo, true);
  }
}

std::string_view ConsolidatedIndex::getZmojis(std::string_view userId) const {
  const auto &dmz = dynamicIndex().metadata().zmojis();
  auto dp = dmz.find(userId);
  if (dp != dmz.end()) {
    return dp->second;
  }
  const auto &fmz = frozenIndex().metadata().zmojis();
  auto fp = fmz.find(userId, frozenIndex().makeLess());
  if (fp != fmz.end()) {
    return frozenIndex().stringPool().toStringView(fp->second);
  }
  return {};
}

int64_t ConsolidatedIndex::getReactionCount(std::string_view reaction, ZgramId relativeTo) const {
  {
    const auto &dmz = dynamicIndex().metadata().reactionCounts();
    auto dp = dmz.find(reaction);
    if (dp != dmz.end()) {
      auto dInner = dp->second.lower_bound(relativeTo);
      if (dInner != dp->second.end()) {
        return dInner->second;
      }
    }
  }

  const auto &fmz = frozenIndex().metadata().reactionCounts();
  auto fp = fmz.find(reaction, frozenIndex().makeLess());
  if (fp != fmz.end()) {
    auto fInner = fp->second.lower_bound(relativeTo);
    if (fInner != fp->second.end()) {
      return fInner->second;
    }
  }
  return 0;
}

namespace {
size_t getPlusPlusHelper(const FrozenStringPool &fsp, const FrozenMetadata::plusPluses_t &plusPluses,
    ZgramId zgramId, std::string_view key) {
  frozenStringRef_t fsr;
  if (!fsp.tryFind(key, &fsr)) {
    return 0;
  }
  auto ip = plusPluses.find(fsr);
  if (ip == plusPluses.end()) {
    return 0;
  }
  const auto &vec = ip->second;
  auto vecp = std::upper_bound(vec.begin(), vec.end(), zgramId);
  return vecp - vec.begin();
}

size_t getPlusPlusHelper(const DynamicMetadata::plusPluses_t &plusPluses, ZgramId zgramId,
    std::string_view key) {
  auto ip = plusPluses.find(key);
  if (ip == plusPluses.end()) {
    return 0;
  }
  const auto &vec = ip->second;
  auto vecp = std::upper_bound(vec.begin(), vec.end(), zgramId);
  return vecp - vec.begin();
}
}  // namespace

int64_t ConsolidatedIndex::getPlusPlusCountAfter(ZgramId zgramId, std::string_view key) const {
  const auto &fsp = frozenIndex().stringPool();
  auto fpRank = getPlusPlusHelper(fsp, frozenIndex().metadata().plusPluses(), zgramId, key);
  auto fmRank = getPlusPlusHelper(fsp, frozenIndex().metadata().minusMinuses(), zgramId, key);
  auto dpRank = getPlusPlusHelper(dynamicIndex().metadata().plusPluses(), zgramId, key);
  auto dmRank = getPlusPlusHelper(dynamicIndex().metadata().minusMinuses(), zgramId, key);

  return int64_t(fpRank) - int64_t(fmRank) + int64_t(dpRank) - int64_t(dmRank);
}

std::set<std::string> ConsolidatedIndex::getPlusPlusKeys(ZgramId zgramId) const {
  std::set<std::string> result;
  const auto &fsp = frozenIndex().stringPool();
  const FrozenVector<frozenStringRef_t> *mentions;
  if (frozenIndex().metadata().plusPlusKeys().tryFind(zgramId, &mentions)) {
    for (const auto &fsr : *mentions) {
      auto sv = fsp.toStringView(fsr);
      result.insert(std::string(sv));
    }
  }

  const auto &ppKeys = dynamicIndex().metadata().plusPlusKeys();
  auto ip = ppKeys.find(zgramId);
  if (ip  != ppKeys.end()) {
    result.insert(ip->second.begin(), ip->second.end());
  }
  return result;
}

bool ConsolidatedIndex::tryDetermineLogged(const MetadataRecord &mr, bool *isLogged,
    const FailFrame &ff) {
  *isLogged = true;  // Optimistically assume logged.
  auto zgramId = getZgramId(mr);
  zgramOff_t zgramOff;
  if (zgramId.has_value() && !tryFind(*zgramId, &zgramOff)) {
    return ff.failf(HERE, "Failed to look up zgram id %o", zgramId);
  }
  *isLogged = ExpandedFileKey(getZgramInfo(zgramOff).location().fileKey()).isLogged();
  return true;
}

bool ConsolidatedIndex::tryAppendAndFlush(std::string_view logged, std::string_view unlogged, const FailFrame &ff) {
  return tryAppendAndFlushHelper(logged, &loggedState_, ff.nest(HERE)) &&
      tryAppendAndFlushHelper(unlogged, &unloggedState_, ff.nest(HERE));
}

namespace {
bool tryAppendAndFlushHelper(std::string_view buffer, internal::DynamicFileState *state,
    const FailFrame &ff) {
  if (!nsunix::tryWriteAll(state->fileCloser().get(), buffer.data(), buffer.size(), ff.nest(HERE))) {
    return false;
  }
  state->advance(buffer.size());
  return true;
}

bool tryReadAllDynamicFiles(const PathMaster &pm,
    const std::vector<IntraFileRange<true>> &loggedKeys,
    const std::vector<IntraFileRange<false>> &unloggedKeys,
    std::vector<DynamicIndex::logRecordAndLocation_t> *result, const FailFrame &ff) {
  // We have an ordering problem. For a given DateAndPartKey, zgrams might be arbitrarily
  // distributed between logged and unlogged. For example zgram 100 might be logged, and zgram 101
  // might be unlogged, then zgram 102 might be logged again. But we need to add them to the index
  // sequential: we simply can't add all the logged then all the unlogged, for example.
  // To deal with this, we pull them all in and then sort them using a stable sort.
  struct extractor_t {
    void operator()(const Zephyrgram &zg) { zg_ = &zg; }
    void operator()(const MetadataRecord &mr) { mr_ = &mr; }

    const Zephyrgram *zg_ = nullptr;
    const MetadataRecord *mr_ = nullptr;
  };

  // Zgrams ordered before Metadata. Zgrams are ordered by id. Metadata are all equivalent,
  // but their ordering is preserved thanks to the stable sort.
  auto myLess = [](const LogParser::logRecordAndLocation_t &lhs,
      const LogParser::logRecordAndLocation_t &rhs) {
    extractor_t lExtractor, rExtractor;
    std::visit(lExtractor, lhs.first.payload());
    std::visit(rExtractor, rhs.first.payload());

    // Is lhs < rhs ?
    // case 1: Zgram vs Zgram: lhs.id < rhs.id
    // case 2: Zgram vs Metadata: true
    // case 3: Metadata vs Zgram: false
    // case 4: Metadata vs Metadata: false
    if (lExtractor.mr_ != nullptr) {
      // Cases 3 and 4
      return false;
    }

    // lhs must be Zgram
    if (rExtractor.mr_ != nullptr) {
      // Case 2.
      return true;
    }

    // lhs and rhs must be Zgram
    return lExtractor.zg_->zgramId() < rExtractor.zg_->zgramId();
  };

  for (const auto &ifr : loggedKeys) {
    auto path = pm.getPlaintextPath(ifr.fileKey().key());
    if (!LogParser::tryParseLogFile(pm, ifr.fileKey().key(), result, ff.nest(HERE))) {
      return false;
    }
  }

  for (const auto &ifr : unloggedKeys) {
    auto path = pm.getPlaintextPath(ifr.fileKey().key());
    if (!LogParser::tryParseLogFile(pm, ifr.fileKey().key(), result, ff.nest(HERE))) {
      return false;
    }
  }

  std::stable_sort(result->begin(), result->end(), myLess);
  return true;
}
}  // namespace

namespace internal {
bool DynamicFileState::tryCreate(const PathMaster &pm, CompressedFileKey fileKey,
    uint32_t offset, DynamicFileState *result, const FailFrame &ff) {
  FileCloser fc;
  if (!tryCreateOrAppendToLogFile(pm, fileKey, offset, &fc, ff.nest(HERE))) {
    return false;
  }
  *result = DynamicFileState(std::move(fc), fileKey, offset);
  return true;
}
DynamicFileState::DynamicFileState() = default;
DynamicFileState::DynamicFileState(DynamicFileState &&) noexcept = default;
DynamicFileState &DynamicFileState::operator=(DynamicFileState &&) noexcept = default;
DynamicFileState::DynamicFileState(FileCloser fileCloser, CompressedFileKey fileKey, size_t fileSize) :
    fileCloser_(std::move(fileCloser)), fileKey_(fileKey), fileSize_(fileSize) {}
DynamicFileState::~DynamicFileState() = default;
}  // namespace internal

namespace {
bool tryCreateOrAppendToLogFile(const PathMaster &pm, CompressedFileKey fileKey, uint32_t offset,
    FileCloser *fc, const FailFrame &ff) {
  auto fileName = pm.getPlaintextPath(fileKey);
  bool exists;
  if (!nsunix::tryExists(fileName, &exists, ff.nest(HERE))) {
    return false;
  }
  if (exists) {
    struct stat stat = {};
    if (!nsunix::tryOpen(fileName, O_WRONLY | O_APPEND, 0700, fc, ff.nest(HERE)) ||
        !nsunix::tryFstat(fc->get(), &stat, ff.nest(HERE))) {
      return false;
    }
    if (offset != stat.st_size) {
      return ff.failf(HERE, "Expected file to end at %o, but it ends at %o", offset, stat.st_size);
    }
    return true;
  }
  return nsunix::tryEnsureBaseExists(fileName, 0700, ff.nest(HERE)) &&
    nsunix::tryOpen(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0600, fc, ff.nest(HERE));
}
}  // namespace
}  // namespace z2kplus::backend::reverse_index::index
