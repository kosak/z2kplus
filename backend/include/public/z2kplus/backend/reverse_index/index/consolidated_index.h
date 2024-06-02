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
#include <regex>
#include <string>
#include <utility>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/delegate.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/memory/mapped_file.h"
#include "kosak/coding/unix.h"
#include "z2kplus/backend/factories/log_parser.h"
#include "z2kplus/backend/files/keys.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/reverse_index/fields.h"
#include "z2kplus/backend/reverse_index/index/dynamic_index.h"
#include "z2kplus/backend/reverse_index/index/frozen_index.h"
#include "z2kplus/backend/reverse_index/index/zgram_cache.h"
#include "z2kplus/backend/reverse_index/metadata/frozen_metadata.h"
#include "z2kplus/backend/reverse_index/types.h"
#include "z2kplus/backend/shared/magic_constants.h"
#include "z2kplus/backend/shared/plusplus_scanner.h"
#include "z2kplus/backend/shared/profile.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/util/automaton/automaton.h"
#include "z2kplus/backend/util/misc.h"
#include "z2kplus/backend/util/relative.h"

// A ConsolidatedIndex is a wrapper around two indices:
// frozen - A FrozenIndex, the external on-disk index that we map into memory.
// dynamic - A DynamicIndex, having new zgrams and new/modified metadata.
namespace z2kplus::backend::reverse_index::index {
namespace internal {
class DynamicFileState {
  typedef kosak::coding::FailFrame FailFrame;
  typedef kosak::coding::nsunix::FileCloser FileCloser;
  typedef z2kplus::backend::files::CompressedFileKey CompressedFileKey;
  typedef z2kplus::backend::files::PathMaster PathMaster;

public:
  static bool tryCreate(const PathMaster &pm, CompressedFileKey fileKey,
      uint32_t offset, DynamicFileState *result, const FailFrame &ff);

  DynamicFileState();
  DISALLOW_COPY_AND_ASSIGN(DynamicFileState);
  DECLARE_MOVE_COPY_AND_ASSIGN(DynamicFileState);
  ~DynamicFileState();

  void advance(uint32_t bytes) {
    fileSize_ += bytes;
  }

  const FileCloser &fileCloser() const { return fileCloser_; }

  const CompressedFileKey &fileKey() const { return fileKey_; }
  uint32_t fileSize() const { return fileSize_; }

private:
  DynamicFileState(FileCloser fileCloser, CompressedFileKey fileKey, size_t fileSize);

  FileCloser fileCloser_;
  CompressedFileKey fileKey_;
  uint32_t fileSize_ = 0;
};
}  // namespace internal

class ConsolidatedIndex {
  typedef kosak::coding::FailFrame FailFrame;
  typedef kosak::coding::nsunix::FileCloser FileCloser;
  typedef z2kplus::backend::factories::LogParser::logRecordAndLocation_t logRecordAndLocation_t;
  typedef z2kplus::backend::files::PathMaster PathMaster;
  typedef z2kplus::backend::reverse_index::index::DynamicIndex DynamicIndex;
  typedef z2kplus::backend::reverse_index::index::FrozenIndex FrozenIndex;
  typedef z2kplus::backend::reverse_index::metadata::FrozenMetadata FrozenMetadata;
  typedef z2kplus::backend::reverse_index::WordInfo WordInfo;
  typedef z2kplus::backend::reverse_index::ZgramInfo ZgramInfo;
  typedef z2kplus::backend::shared::LogRecord LogRecord;
  typedef z2kplus::backend::shared::MetadataRecord MetadataRecord;
  typedef z2kplus::backend::shared::PlusPlusScanner PlusPlusScanner;
  typedef z2kplus::backend::shared::Profile Profile;
  typedef z2kplus::backend::shared::ZgramCore ZgramCore;
  typedef z2kplus::backend::shared::Zephyrgram Zephyrgram;
  typedef z2kplus::backend::shared::ZgramId ZgramId;
  typedef z2kplus::backend::util::automaton::FiniteAutomaton FiniteAutomaton;

  template<bool IsLogged>
  using FilePosition = z2kplus::backend::files::FilePosition<IsLogged>;

  template<typename T>
  using MappedFile =  kosak::coding::memory::MappedFile<T>;

  template<typename R, typename ...Args>
  using Delegate = kosak::coding::Delegate<R, Args...>;
public:
  typedef DynamicIndex::ppDeltaMap_t ppDeltaMap_t;

  static bool tryCreate(std::shared_ptr<PathMaster> pm,
      std::chrono::system_clock::time_point now,
      ConsolidatedIndex *result, const FailFrame &ff);

  static bool tryCreate(std::shared_ptr<PathMaster> pm,
      const FilePosition<true> &loggedStart, const FilePosition<false> &unloggedStart,
      MappedFile<FrozenIndex> frozenIndex, ConsolidatedIndex *result, const FailFrame &ff);

  ConsolidatedIndex();
  ConsolidatedIndex(ConsolidatedIndex &&other) noexcept;
  ConsolidatedIndex &operator=(ConsolidatedIndex &&other) noexcept;
  ~ConsolidatedIndex();

  bool tryAddZgrams(std::chrono::system_clock::time_point now, const Profile &profile,
      std::vector<ZgramCore> &&zgcs, ppDeltaMap_t *deltaMap, std::vector<Zephyrgram> *zgrams,
      const FailFrame &ff);

  bool tryAddMetadata(std::vector<MetadataRecord> &&records, ppDeltaMap_t *deltaMap,
      std::vector<MetadataRecord> *movedRecords, const FailFrame &ff);

  bool tryAddForBootstrap(const std::vector<logRecordAndLocation_t> &records, const FailFrame &ff);

  void findMatching(const FiniteAutomaton &dfa,
      const kosak::coding::Delegate<void, const wordOff_t *, const wordOff_t *> &callback) const;

  bool tryCheckpoint(std::chrono::system_clock::time_point now,
      FilePosition<true> *loggedPosition, FilePosition<false> *unloggedPosition,
      const FailFrame &ff);

  bool tryFind(ZgramId id, zgramOff_t *result) const;
  zgramOff_t lowerBound(ZgramId id) const;
  zgramOff_t lowerBound(uint64_t timestamp) const;
  const ZgramInfo &getZgramInfo(zgramOff_t zgramOff) const;
  const WordInfo &getWordInfo(wordOff_t wordOff) const;

  ZgramId zgramEnd() const;

  zgramOff_t zgramEndOff() const {
    return zgramOff_t(static_cast<uint32>(zgramInfoSize()));
  }

  void getMetadataFor(ZgramId zgramId, std::vector<MetadataRecord> *result) const;
  std::string_view getZmojis(std::string_view userId) const;
  int64_t getReactionCount(std::string_view reaction, ZgramId relativeTo) const;
  int64_t getPlusPlusCountAfter(ZgramId zgramId, std::string_view key) const;
  std::set<std::string> getPlusPlusKeys(ZgramId zgramId) const;

  void getReactionsFor(ZgramId zgramId, std::vector<shared::zgMetadata::Reaction> *result) const;
  void getZgramRevsFor(ZgramId zgramId, std::vector<shared::zgMetadata::ZgramRevision> *result) const;
  void getRefersToFor(ZgramId zgramId, std::vector<shared::zgMetadata::ZgramRefersTo> *result) const;

  size_t zgramInfoSize() const {
    return frozenIndex_.get()->zgramInfos().size() + dynamicIndex_.zgramInfos().size();
  }

  size_t wordInfoSize() const {
    return frozenIndex_.get()->wordInfos().size() + dynamicIndex_.wordInfos().size();
  }

  const PathMaster &pm() const { return *pm_; }

  const FrozenIndex &frozenIndex() const { return *frozenIndex_.get(); }
  const DynamicIndex &dynamicIndex() const { return dynamicIndex_; }

  ZgramCache &zgramCache() { return zgramCache_; }

private:
  ConsolidatedIndex(std::shared_ptr<PathMaster> &&pm,
      MappedFile<FrozenIndex> &&frozenIndex, internal::DynamicFileState &&loggedState,
      internal::DynamicFileState &&unloggedState);

  bool tryAddZgramsHelper(std::chrono::system_clock::time_point now, const Profile &profile,
      std::vector<ZgramCore> &&zgcs, std::vector<Zephyrgram> *zgrams, const FailFrame &ff);
  bool tryAddMetadataHelper(std::vector<MetadataRecord> &&records, std::vector<MetadataRecord> *movedRecords,
      const FailFrame &ff);

  bool tryDetermineLogged(const MetadataRecord &mr, bool *isLogged, const FailFrame &ff);
  bool tryAppendAndFlush(std::string_view logged, std::string_view unlogged, const FailFrame &ff);

  std::shared_ptr<PathMaster> pm_;

  // Zgrams and metadata that have been digested and stored in the on-disk format.
  MappedFile<FrozenIndex> frozenIndex_;
  // Freshly arrived zephyrgrams and metadata.
  DynamicIndex dynamicIndex_;

  // Log for the freshly-arrived logged zgrams and metadata
  internal::DynamicFileState loggedState_;
  // Log for the freshly-arrived unlogged zgrams and metadata
  internal::DynamicFileState unloggedState_;

  ZgramCache zgramCache_;
};
}  // namespace z2kplus::backend::reverse_index::index
