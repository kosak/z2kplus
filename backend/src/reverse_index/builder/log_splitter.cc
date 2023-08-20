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

#include "z2kplus/backend/reverse_index/builder/log_splitter.h"

#include "kosak/coding/memory/buffered_writer.h"
#include "kosak/coding/sorting/sort_manager.h"
#include "kosak/coding/text/misc.h"
#include "z2kplus/backend/reverse_index/builder/schemas.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/tuple_serializer.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/util.h"
#include "z2kplus/backend/shared/magic_constants.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::makeReservedVector;
using kosak::coding::memory::BufferedWriter;
using kosak::coding::memory::MappedFile;
using kosak::coding::sorting::KeyOptions;
using kosak::coding::sorting::SortOptions;
using kosak::coding::streamf;
using kosak::coding::stringf;
using kosak::coding::toString;
using kosak::coding::text::Splitter;
using kosak::coding::text::trim;
using kosak::coding::ParseContext;
using kosak::coding::sorting::SortManager;
using kosak::coding::nsunix::FileCloser;
using z2kplus::backend::files::FileKey;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::shared::LogRecord;
using z2kplus::backend::shared::MetadataRecord;
using z2kplus::backend::shared::Zephyrgram;
using z2kplus::backend::shared::ZgramCore;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::reverse_index::builder::tuple_iterators::tupleSerializer::tryAppendTuple;
using z2kplus::backend::reverse_index::builder::tuple_iterators::traits::TruncateTuple;

#define HERE KOSAK_CODING_HERE

namespace filenames = z2kplus::backend::shared::magicConstants::filenames;
namespace magicConstants = z2kplus::backend::shared::magicConstants;
namespace nsunix = kosak::coding::nsunix;
namespace schemas = z2kplus::backend::reverse_index::builder::schemas;
namespace userMetadata = z2kplus::backend::shared::userMetadata;
namespace zgMetadata = z2kplus::backend::shared::zgMetadata;

namespace z2kplus::backend::reverse_index::builder {
namespace {

struct SplitterInputs {
  SplitterInputs(std::string loggedZgrams, std::string unloggedZgrams, std::string reactionsByZgramId,
      std::string reactionsByReaction, std::string zgramRevisions, std::string zgramRefersTo,
      std::string zmojis) : loggedZgrams_(std::move(loggedZgrams)), unloggedZgrams_(std::move(unloggedZgrams)),
      reactionsByZgramId_(std::move(reactionsByZgramId)), reactionsByReaction_(std::move(reactionsByReaction)),
      zgramRevisions_(std::move(zgramRevisions)), zgramRefersTo_(std::move(zgramRefersTo)), zmojis_(std::move(zmojis)) {}

  std::string loggedZgrams_;
  std::string unloggedZgrams_;
  std::string reactionsByZgramId_;
  std::string reactionsByReaction_;
  std::string zgramRevisions_;
  std::string zgramRefersTo_;
  std::string zmojis_;
};

struct NameAndWriter {
  std::string outputName_;
  BufferedWriter writer_;
};

struct SplitterThread {
  static bool tryCreate(size_t shard, const PathMaster *pm, const SplitterInputs &sis,
      const FileKey *startKey, const FileKey *endKey, std::shared_ptr<SplitterThread> *result,
      const FailFrame &ff);

  SplitterThread(size_t shard, const PathMaster *pm, const FileKey *startKey, const FileKey *endKey,
      NameAndWriter logged, NameAndWriter unlogged, NameAndWriter reactionsByZgramId,
      NameAndWriter reactionsByReaction, NameAndWriter zgramRevs, NameAndWriter zgramRefersTo,
      NameAndWriter zmojis);
  DISALLOW_COPY_AND_ASSIGN(SplitterThread);
  DISALLOW_MOVE_COPY_AND_ASSIGN(SplitterThread);
  ~SplitterThread();

  bool tryFinish(const FailFrame &ff);

  static void run(std::shared_ptr<SplitterThread> self);
  bool tryRunHelper(const FailFrame &ff);

  size_t shard_;
  const PathMaster *pm_ = nullptr;
  const FileKey *startKey_ = nullptr;
  const FileKey *endKey_ = nullptr;

  NameAndWriter logged_;
  NameAndWriter unlogged_;
  NameAndWriter reactionsByZgramId_;
  NameAndWriter reactionsByReaction_;
  NameAndWriter zgramRevs_;
  NameAndWriter zgramRefersTo_;
  NameAndWriter zmojis_;

  std::optional<ZgramId> prevLoggedZgramId_;
  std::optional<ZgramId> prevUnloggedZgramId_;

  std::optional<std::string> error_;

  std::thread thread_;
};

class SplitterVisitor {
public:
  SplitterVisitor(SplitterThread *owner, const FileKey &fileKey, size_t offset, size_t size,
      const FailFrame *ff);
  ~SplitterVisitor() = default;

  bool operator()(const Zephyrgram &o);
  bool operator()(const MetadataRecord &o);
  bool operator()(const zgMetadata::Reaction &o);
  bool operator()(const zgMetadata::ZgramRevision &o);
  bool operator()(const zgMetadata::ZgramRefersTo &o);
  bool operator()(const userMetadata::Zmojis &o);

private:
  template<typename ...Args>
  bool appendHelper(BufferedWriter *bw, const std::tuple<Args...> &tuple) const;

  SplitterThread *owner_ = nullptr;
  FileKey fileKey_;
  size_t offset_ = 0;
  size_t size_ = 0;
  const FailFrame *ff_;
};
}  // namespace

bool LogSplitter::split(const PathMaster &pm, std::vector<FileKey> fileKeys,
    size_t numShards, LogSplitterResult *result, const FailFrame &ff) {
  auto loggedZgrams = pm.getScratchPathFor(filenames::loggedZgrams);
  auto unloggedZgrams = pm.getScratchPathFor(filenames::unloggedZgrams);
  auto reactionsByZgramId = pm.getScratchPathFor(filenames::reactionsByZgramId);
  auto reactionsByReaction = pm.getScratchPathFor(filenames::reactionsByReaction);
  auto zgramRevisions = pm.getScratchPathFor(filenames::zgramRevisions);
  auto zgramRefersTo = pm.getScratchPathFor(filenames::zgramRefersTo);
  auto zmojis = pm.getScratchPathFor(filenames::zmojis);
  SplitterInputs sis(std::move(loggedZgrams), std::move(unloggedZgrams),
      std::move(reactionsByZgramId), std::move(reactionsByReaction), std::move(zgramRevisions),
      std::move(zgramRefersTo), std::move(zmojis));

  std::sort(fileKeys.begin(), fileKeys.end());

  auto shardSize = fileKeys.size() / numShards;
  auto excess = fileKeys.size() % numShards;
  const FileKey *shardStart = fileKeys.data();
  std::vector<std::shared_ptr<SplitterThread>> sts(numShards);
  for (size_t i = 0; i != numShards; ++i) {
    auto bonusWork = excess != 0 ? 1 : 0;
    const auto *shardEnd = shardStart + shardSize + bonusWork;
    excess -= bonusWork;
    if (!SplitterThread::tryCreate(i, &pm, sis, shardStart, shardEnd, &sts[i], ff.nest(HERE))) {
      return false;
    }
    shardStart = shardEnd;
  }

  // Finish all the threads before reporting errors in any of them.
  for (size_t i = 0; i != numShards; ++i) {
    (void)sts[i]->tryFinish(ff.nest(HERE));
  }
  if (!ff.ok()) {
    return false;
  }

  auto gatherAndMoveInputs = [&sts](NameAndWriter SplitterThread::*field) {
    auto result = makeReservedVector<std::string>(sts.size());
    for (const auto &st : sts) {
      result.push_back(std::move((st.get()->*field).outputName_));
    }
    return result;
  };

  auto rxByZInputs = gatherAndMoveInputs(&SplitterThread::reactionsByZgramId_);
  auto rxByRInputs = gatherAndMoveInputs(&SplitterThread::reactionsByReaction_);
  auto zgRevInputs = gatherAndMoveInputs(&SplitterThread::zgramRevs_);
  auto zgRefersToInputs = gatherAndMoveInputs(&SplitterThread::zgramRefersTo_);
  auto zmojiInputs = gatherAndMoveInputs(&SplitterThread::zmojis_);
  // These are kept separate so they can be processed in separate threads in a later stage.
  auto loggedZgramInputs = gatherAndMoveInputs(&SplitterThread::logged_);
  auto unloggedZgramInputs = gatherAndMoveInputs(&SplitterThread::unlogged_);

  SortOptions sortOptions(true, false, defaultFieldSeparator, true);
  SortManager rxByZSorter;
  SortManager rxByRSorter;
  SortManager zgRevSorter;
  SortManager zgRefersToSorter;
  SortManager zmojiSorter;

  // Start all the sorts (in parallel)
  if (!SortManager::tryCreate(sortOptions, schemas::ReactionsByZgramId::keyOptions(),
          rxByZInputs, sis.reactionsByZgramId_, &rxByZSorter, ff.nest(HERE)) ||
      !SortManager::tryCreate(sortOptions, schemas::ReactionsByReaction::keyOptions(),
          rxByRInputs, sis.reactionsByReaction_, &rxByRSorter, ff.nest(HERE)) ||
      !SortManager::tryCreate(sortOptions, schemas::ZgramRevisions::keyOptions(),
          zgRevInputs, sis.zgramRevisions_, &zgRevSorter, ff.nest(HERE)) ||
      !SortManager::tryCreate(sortOptions, schemas::ZgramRefersTos::keyOptions(),
          zgRefersToInputs, sis.zgramRefersTo_, &zgRefersToSorter, ff.nest(HERE)) ||
      !SortManager::tryCreate(sortOptions, schemas::ZmojisRevisions::keyOptions(),
          zmojiInputs, sis.zmojis_, &zmojiSorter, ff.nest(HERE))) {
    return false;
  }

  // Finish all the sorts
  if (!rxByZSorter.tryFinish(ff.nest(HERE)) ||
      !rxByRSorter.tryFinish(ff.nest(HERE)) ||
      !zgRevSorter.tryFinish(ff.nest(HERE)) ||
      !zgRefersToSorter.tryFinish(ff.nest(HERE)) ||
      !zmojiSorter.tryFinish(ff.nest(HERE))) {
    return false;
  }

  *result = LogSplitterResult(
      std::move(loggedZgramInputs), std::move(unloggedZgramInputs),
      std::move(sis.reactionsByZgramId_), std::move(sis.reactionsByReaction_),
      std::move(sis.zgramRevisions_), std::move(sis.zgramRefersTo_), std::move(sis.zmojis_));
  return true;
}

namespace {
bool SplitterThread::tryCreate(size_t shard, const PathMaster *pm, const SplitterInputs &sis,
    const FileKey *startKey, const FileKey *endKey, std::shared_ptr<SplitterThread> *result,
    const FailFrame &ff) {
  auto createBw = [](size_t shard, const std::string &filename, NameAndWriter *result,
      const FailFrame &ff) {
    result->outputName_ = stringf("%o.presorted.%o", filename, shard);
    FileCloser fc;
    if (!nsunix::tryOpen(result->outputName_, O_CREAT | O_WRONLY | O_TRUNC, 0644, &fc, ff.nest(HERE))) {
      return false;
    }
    result->writer_ = BufferedWriter(std::move(fc));
    return true;
  };
  NameAndWriter logged;
  NameAndWriter unlogged;
  NameAndWriter reactionsByZgramId;
  NameAndWriter reactionsByReaction;
  NameAndWriter zgramRevs;
  NameAndWriter zgramRefersTo;
  NameAndWriter zmojis;
  if (!createBw(shard, sis.loggedZgrams_, &logged, ff.nest(HERE)) ||
      !createBw(shard, sis.unloggedZgrams_, &unlogged, ff.nest(HERE)) ||
      !createBw(shard, sis.reactionsByZgramId_, &reactionsByZgramId, ff.nest(HERE)) ||
      !createBw(shard, sis.reactionsByReaction_, &reactionsByReaction, ff.nest(HERE)) ||
      !createBw(shard, sis.zgramRevisions_, &zgramRevs, ff.nest(HERE)) ||
      !createBw(shard, sis.zgramRefersTo_, &zgramRefersTo, ff.nest(HERE)) ||
      !createBw(shard, sis.zmojis_, &zmojis, ff.nest(HERE))) {
    return false;
  }

  auto st = std::make_shared<SplitterThread>(shard, pm, startKey, endKey,
      std::move(logged), std::move(unlogged), std::move(reactionsByZgramId),
      std::move(reactionsByReaction), std::move(zgramRevs), std::move(zgramRefersTo),
      std::move(zmojis));
  st->thread_ = std::thread(&run, st);
  *result = std::move(st);
  return true;
}

void SplitterThread::run(std::shared_ptr<SplitterThread> self) {
  FailRoot fr;
  streamf(std::cerr, "Splitter thread %o waking up\n", self->shard_);
  if (!self->tryRunHelper(fr.nest(HERE))) {
    self->error_ = toString(fr);
  }
  streamf(std::cerr, "Splitter thread %o shutting down\n", self->shard_);
}

SplitterThread::SplitterThread(size_t shard, const PathMaster *pm, const FileKey *startKey,
    const FileKey *endKey, NameAndWriter logged, NameAndWriter unlogged,
    NameAndWriter reactionsByZgramId, NameAndWriter reactionsByReaction,
    NameAndWriter zgramRevs, NameAndWriter zgramRefersTo, NameAndWriter zmojis) : shard_(shard),
    pm_(pm), startKey_(startKey), endKey_(endKey), logged_(std::move(logged)),
    unlogged_(std::move(unlogged)), reactionsByZgramId_(std::move(reactionsByZgramId)),
    reactionsByReaction_(std::move(reactionsByReaction)), zgramRevs_(std::move(zgramRevs)),
    zgramRefersTo_(std::move(zgramRefersTo)), zmojis_(std::move(zmojis)) {}

SplitterThread::~SplitterThread() = default;

bool SplitterThread::tryRunHelper(const FailFrame &ff) {
  // Split records into various individual files so they can be sorted and then digested.
  for (const auto *current = startKey_; current != endKey_; ++current) {
    const auto &fileKey = *current;
    auto pathName = pm_->getPlaintextPath(fileKey);
    MappedFile<char> mf;
    if (!mf.tryMap(pathName, false, ff.nest(HERE))) {
      return false;
    }
    std::string_view recordText(mf.get(), mf.byteSize());
    auto splitter = Splitter::ofRecords(recordText, '\n');
    std::string_view record;
    size_t offset = 0;
    while (splitter.moveNext(&record)) {
      record = trim(record);
      ParseContext ctx(record);
      LogRecord lr;
      using kosak::coding::tryParseJson;
      if (!tryParseJson(&ctx, &lr, ff.nest(HERE))) {
        return false;
      }
      auto ff2 = ff.nest(HERE);
      SplitterVisitor visitor(this, fileKey, offset, record.size(), &ff2);
      if (!std::visit(visitor, lr.payload())) {
        return false;
      }
      offset += record.size() + 1;
    }
  }
  return true;
}

bool SplitterThread::tryFinish(const FailFrame &ff) {
  thread_.join();
  if (!logged_.writer_.tryClose(ff.nest(HERE)) ||
      !unlogged_.writer_.tryClose(ff.nest(HERE)) ||
      !reactionsByZgramId_.writer_.tryClose(ff.nest(HERE)) ||
      !reactionsByReaction_.writer_.tryClose(ff.nest(HERE)) ||
      !zgramRevs_.writer_.tryClose(ff.nest(HERE)) ||
      !zgramRefersTo_.writer_.tryClose(ff.nest(HERE)) ||
      !zmojis_.writer_.tryClose(ff.nest(HERE))) {
    return false;
  }

  if (error_.has_value()) {
    return ff.failf(HERE, "%o", error_);
  }
  return true;
}

SplitterVisitor::SplitterVisitor(SplitterThread *owner, const FileKey &fileKey, size_t offset,
    size_t size, const FailFrame *ff) : owner_(owner), fileKey_(fileKey), offset_(offset),
    size_(size), ff_(ff) {}

bool SplitterVisitor::operator()(const Zephyrgram &o) {
  std::optional<ZgramId> *whichPrev;
  BufferedWriter *whichWriter;
  bool expectedLogged;
  if (fileKey_.asExpandedFileKey().isLogged()) {
    whichPrev = &owner_->prevLoggedZgramId_;
    whichWriter = &owner_->logged_.writer_;
    expectedLogged = true;
  } else {
    whichPrev = &owner_->prevUnloggedZgramId_;
    whichWriter = &owner_->unlogged_.writer_;
    expectedLogged = false;
  }
  if (whichPrev->has_value() && *whichPrev >= o.zgramId()) {
    return ff_->failf(HERE, "Zgrams arriving out of order: %o then %o", *whichPrev, o.zgramId());
  }
  if (expectedLogged != o.isLogged()) {
    return ff_->failf(HERE, "Expected zgramId %o to have logged=%o, but found logged=%o",
        o.zgramId(), expectedLogged, o.isLogged());
  }
  *whichPrev = o.zgramId();
  auto row = schemas::Zephyrgram::createTuple(o, fileKey_, offset_, size_);
  return appendHelper(whichWriter, row);
}

bool SplitterVisitor::operator()(const MetadataRecord &o) {
  return std::visit(*this, o.payload());
}

bool SplitterVisitor::operator()(const zgMetadata::Reaction &o) {
  auto rbzRow = schemas::ReactionsByZgramId::createTuple(o);
  auto rbrRow = schemas::ReactionsByReaction::createTuple(o);
  return appendHelper(&owner_->reactionsByZgramId_.writer_, rbzRow) &&
      appendHelper(&owner_->reactionsByReaction_.writer_, rbrRow);
}

bool SplitterVisitor::operator()(const zgMetadata::ZgramRevision &o) {
  auto row = schemas::ZgramRevisions::createTuple(o);
  return appendHelper(&owner_->zgramRevs_.writer_, row);
}

bool SplitterVisitor::operator()(const zgMetadata::ZgramRefersTo &o) {
  auto row = schemas::ZgramRefersTos::createTuple(o);
  return appendHelper(&owner_->zgramRefersTo_.writer_, row);
}

bool SplitterVisitor::operator()(const userMetadata::Zmojis &o) {
  auto row = schemas::ZmojisRevisions::createTuple(o);
  return appendHelper(&owner_->zmojis_.writer_, row);
}

template<typename ...Args>
bool SplitterVisitor::appendHelper(BufferedWriter *bw, const std::tuple<Args...> &tuple) const {
  return tryAppendTuple(tuple, defaultFieldSeparator, bw->getBuffer(), ff_->nest(HERE)) &&
      bw->tryWriteByte(defaultRecordSeparator, ff_->nest(HERE));
}
}  // namespace

LogSplitterResult::LogSplitterResult() = default;
LogSplitterResult::LogSplitterResult(std::vector<std::string> loggedZgrams, std::vector<std::string> unloggedZgrams,
    std::string reactionsByZgramId, std::string reactionsByReaction, std::string zgramRevisions,
    std::string zgramRefersTo, std::string zmojis) : loggedZgrams_(std::move(loggedZgrams)),
    unloggedZgrams_(std::move(unloggedZgrams)), reactionsByZgramId_(std::move(reactionsByZgramId)),
    reactionsByReaction_(std::move(reactionsByReaction)), zgramRevisions_(std::move(zgramRevisions)),
    zgramRefersTo_(std::move(zgramRefersTo)), zmojis_(std::move(zmojis)) {}
LogSplitterResult::LogSplitterResult(LogSplitterResult &&) noexcept = default;
LogSplitterResult &LogSplitterResult::operator=(LogSplitterResult &&) noexcept = default;
LogSplitterResult::~LogSplitterResult() = default;
}  // namespace z2kplus::backend::reverse_index::builder
