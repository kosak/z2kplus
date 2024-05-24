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

#include "kosak/coding/containers/slice.h"
#include "z2kplus/backend/files/keys.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/queryparsing/util.h"
#include "z2kplus/backend/reverse_index/fields.h"
#include "z2kplus/backend/reverse_index/index/frozen_index.h"
#include "z2kplus/backend/reverse_index/metadata/dynamic_metadata.h"
#include "z2kplus/backend/reverse_index/trie/dynamic_trie.h"
#include "z2kplus/backend/reverse_index/types.h"
#include "z2kplus/backend/shared/plusplus_scanner.h"
#include "z2kplus/backend/shared/zephyrgram.h"

namespace z2kplus::backend::reverse_index::index {
class DynamicIndex {
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::files::FileKey FileKey;
  typedef z2kplus::backend::files::IntraFileRange IntraFileRange;
  typedef z2kplus::backend::files::PathMaster PathMaster;
  typedef z2kplus::backend::reverse_index::index::FrozenIndex FrozenIndex;
  typedef z2kplus::backend::reverse_index::metadata::DynamicMetadata DynamicMetadata;
  typedef z2kplus::backend::reverse_index::trie::DynamicTrie DynamicTrie;
  typedef z2kplus::backend::queryparsing::WordSplitter WordSplitter;
  typedef z2kplus::backend::shared::MetadataRecord MetadataRecord;
  typedef z2kplus::backend::shared::PlusPlusScanner PlusPlusScanner;
  typedef z2kplus::backend::shared::LogRecord LogRecord;
  typedef z2kplus::backend::shared::ZgramId ZgramId;
  typedef z2kplus::backend::shared::Zephyrgram Zephyrgram;
  typedef z2kplus::backend::shared::ZgramCore ZgramCore;

  template<typename T>
  using Slice = kosak::coding::containers::Slice<T>;

public:
  typedef std::pair<LogRecord, IntraFileRange> logRecordAndLocation_t;
  typedef std::map<ZgramId, PlusPlusScanner::ppDeltas_t> ppDeltaMap_t;

  DynamicIndex();
  DynamicIndex(DynamicTrie &&trie, std::vector<ZgramInfo> &&zgramInfos,
      std::vector<WordInfo> &&wordInfos, DynamicMetadata &&metadata);
  DISALLOW_COPY_AND_ASSIGN(DynamicIndex);
  DECLARE_MOVE_COPY_AND_ASSIGN(DynamicIndex);
  ~DynamicIndex();

  bool tryAddLogRecords(const FrozenIndex &frozenSide,
      const std::vector<logRecordAndLocation_t> &items, const FailFrame &ff);
  bool tryAddZgrams(const FrozenIndex &frozenSide,
      const Slice<const Zephyrgram> &zgrams, const Slice<const IntraFileRange> &locations, const FailFrame &ff);
  bool tryAddMetadata(const FrozenIndex &frozenSide,
      const Slice<const MetadataRecord> &items, const FailFrame &ff);

  void batchUpdatePlusPlusCounts(const ppDeltaMap_t &deltaMap);

  const DynamicTrie &trie() const { return trie_; }
  const std::vector<ZgramInfo> &zgramInfos() const { return zgramInfos_; }
  const std::vector<WordInfo> &wordInfos() const { return wordInfos_; }
  DynamicMetadata &metadata() { return metadata_; }
  const DynamicMetadata &metadata() const { return metadata_; }

private:
  bool tryAddZgram(const FrozenIndex &frozenSide, const Zephyrgram &zg, const IntraFileRange &location,
      std::vector<std::string_view> *wordStorage, std::u32string *char32Storage, const FailFrame &ff);

  DynamicTrie trie_;
  std::vector<ZgramInfo> zgramInfos_;
  std::vector<WordInfo> wordInfos_;
  DynamicMetadata metadata_;

  friend std::ostream &operator<<(std::ostream &s, const DynamicIndex &o);
};
}  // namespace z2kplus::backend::reverse_index::index
