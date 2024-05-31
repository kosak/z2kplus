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

#include "z2kplus/backend/reverse_index/index/dynamic_index.h"

#include <string>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/text/conversions.h"

using kosak::coding::FailFrame;
using kosak::coding::streamf;
using kosak::coding::text::tryConvertUtf8ToUtf32;
using z2kplus::backend::reverse_index::metadata::FrozenMetadata;
using z2kplus::backend::shared::Zephyrgram;

#define HERE KOSAK_CODING_HERE

namespace zgMetadata = z2kplus::backend::shared::zgMetadata;
namespace userMetadata = z2kplus::backend::shared::userMetadata;

namespace z2kplus::backend::reverse_index::index {

DynamicIndex::DynamicIndex() = default;
DynamicIndex::DynamicIndex(DynamicTrie &&trie, std::vector<ZgramInfo> &&zgramInfos,
    std::vector<WordInfo> &&wordInfos, DynamicMetadata &&metadata) : trie_(std::move(trie)),
    zgramInfos_(std::move(zgramInfos)), wordInfos_(std::move(wordInfos)),
    metadata_(std::move(metadata)) {}
DynamicIndex::DynamicIndex(DynamicIndex &&other) noexcept = default;
DynamicIndex &DynamicIndex::operator=(DynamicIndex &&other) noexcept = default;
DynamicIndex::~DynamicIndex() = default;

bool DynamicIndex::tryAddLogRecords(const FrozenIndex &frozenSide,
    const std::vector<logRecordAndLocation_t> &items, const FailFrame &ff) {
  struct visitor_t {
    visitor_t(DynamicIndex *self, const FrozenIndex &frozenSide, const LogLocation &location,
        const FailFrame *f2) :
      self_(self), frozenSide_(frozenSide), location_(location), f2_(f2) {
    }

    bool operator()(const Zephyrgram &zg) {
      auto zgSlice = Slice<const Zephyrgram>(&zg, 1);
      auto locSlice = Slice<const LogLocation>(&location_, 1);
      return self_->tryAddZgrams(frozenSide_, zgSlice, locSlice, f2_->nest(HERE));
    }

    bool operator()(const MetadataRecord &mr) {
      auto mdSlice = Slice<const MetadataRecord>(&mr, 1);
      return self_->tryAddMetadata(frozenSide_, mdSlice, f2_->nest(HERE));
    }

    DynamicIndex *self_;
    const FrozenIndex &frozenSide_;
    const LogLocation &location_;
    const FailFrame *f2_ = nullptr;
  };
  auto ff2 = ff.nest(HERE);
  for (const auto &item : items) {
    visitor_t visitor(this, frozenSide, item.second, &ff2);
    if (!std::visit(visitor, std::move(item.first.payload()))) {
      return false;
    }
  }
  return true;
}

bool DynamicIndex::tryAddZgrams(const FrozenIndex &frozenSide,
    const Slice<const Zephyrgram> &zgrams, const Slice<const LogLocation> &locations,
    const FailFrame &ff) {
  if (zgrams.size() != locations.size()) {
    return ff.failf(HERE, "zgrams.size (%o) != locations.size (%o)", zgrams.size(), locations.size());
  }
  std::vector<std::string_view> wordStorage;
  std::u32string char32Storage;
  for (size_t i = 0; i != zgrams.size(); ++i) {
    if (!tryAddZgram(frozenSide, zgrams[i], locations[i], &wordStorage, &char32Storage,
        ff.nest(HERE))) {
      return false;
    }
  }
  return true;
}

bool DynamicIndex::tryAddZgram(const FrozenIndex &frozenSide, const Zephyrgram &zg,
    const LogLocation &location, std::vector<std::string_view> *wordStorage,
    std::u32string *char32Storage, const FailFrame &ff) {
  if (!zgramInfos_.empty() && zg.zgramId() <= zgramInfos_.back().zgramId()) {
    return ff.failf(HERE, "Nonincreasing ids: went from %o to %o", zgramInfos_.back().zgramId(), zg.zgramId());
  }
  wordOff_t initialWordOff(frozenSide.wordInfos().size() + wordInfos_.size());

  auto tryAddWords = [this, &frozenSide](FieldTag fieldTag,
      const std::vector<std::string_view> &words, std::u32string *c32s, const FailFrame &ff2) {
    zgramOff_t zgramOff(frozenSide.zgramInfos().size() + zgramInfos_.size());
    wordOff_t wordOff(frozenSide.wordInfos().size() + wordInfos_.size());
    for (const auto &word : words) {
      c32s->clear();
      if (!tryConvertUtf8ToUtf32(word, c32s, ff2.nest(HERE))) {
        return false;
      }
      wordInfos_.emplace_back(zgramOff, fieldTag);
      trie_.insert(*c32s, &wordOff, 1);
      ++wordOff;
    }
    return true;
  };

  const auto &zgc = zg.zgramCore();
  std::array<const std::string_view, 4> fields = {
      zg.sender(), zg.signature(), zgc.instance(), zgc.body()
  };
  std::array<FieldTag, 4> fieldTags = {
      FieldTag::sender, FieldTag::signature, FieldTag::instance, FieldTag::body
  };
  std::array<size_t, 4> sizes = {0};
  static_assert(fields.size() == fieldTags.size());
  static_assert(fields.size() == sizes.size());
  for (size_t i = 0; i < fields.size(); ++i) {
    wordStorage->clear();
    WordSplitter::split(fields[i], wordStorage);
    sizes[i] = wordStorage->size();
    if (!tryAddWords(fieldTags[i], *wordStorage, char32Storage, ff.nest(HERE))) {
      return false;
    }
  }

  static_assert(sizes.size() == 4);
  ZgramInfo zgInfo;
  if (!ZgramInfo::tryCreate(zg.timesecs(), location, initialWordOff, zg.zgramId(), sizes[0],
      sizes[1], sizes[2], sizes[3], &zgInfo, ff.nest(HERE))) {
    return false;
  }
  zgramInfos_.push_back(zgInfo);
  return true;
}

bool DynamicIndex::tryAddMetadata(const FrozenIndex &frozenSide,
    const Slice<const MetadataRecord> &items, const FailFrame &ff) {
  struct visitor_t {
    visitor_t(DynamicIndex *self, const FrozenIndex &frozen, const FailFrame *f2) :
        frozen_(frozen), f2_(f2), dynamicMetadata_(&self->metadata_) {
    }

    bool operator()(const zgMetadata::Reaction &o) const {
      return dynamicMetadata_->tryAddHelper(frozen_, o, f2_->nest(HERE));
    }

    bool operator()(const zgMetadata::ZgramRevision &o) const {
      return dynamicMetadata_->tryAddHelper(frozen_, o, f2_->nest(HERE));
    }

    bool operator()(const zgMetadata::ZgramRefersTo &o) const {
      return dynamicMetadata_->tryAddHelper(frozen_, o, f2_->nest(HERE));
    }

    bool operator()(const userMetadata::Zmojis &o) const {
      return dynamicMetadata_->tryAddHelper(frozen_, o, f2_->nest(HERE));
    }

    const FrozenIndex &frozen_;
    const FailFrame *f2_ = nullptr;
    DynamicMetadata *dynamicMetadata_ = nullptr;
  };

  for (const auto &mr : items) {
    auto f2 = ff.nest(HERE);
    visitor_t visitor(this, frozenSide, &f2);
    if (!std::visit(visitor, mr.payload())) {
      return false;
    }
  }
  return true;
}


void DynamicIndex::batchUpdatePlusPlusCounts(const ppDeltaMap_t &deltaMap) {
  std::map<std::string, std::map<ZgramId, int64_t>> transposedMap;
  for (const auto &[zgramId, inner]: deltaMap) {
    for (auto [key, count]: inner) {
      transposedMap[key][zgramId] = count;
    }
  }

  // Only works because plusPluses_t and minusMinuses_t are the same type
  auto addEntries = [](DynamicMetadata::plusPluses_t *which, const std::string &key,
      ZgramId zgramId, size_t count) {
    // Find or insert an entry for 'key'. Avoid a string allocation if you can.
    auto ip = which->find(key);
    if (ip == which->end()) {
      ip = which->try_emplace(std::string(key)).first;
    }
    // Insert 'countToUse' copies of zgramId
    auto &vec = ip->second;
    auto vecp = std::upper_bound(vec.begin(), vec.end(), zgramId);
    vec.insert(vecp, count, zgramId);
  };

  for (const auto &[key, inner] : transposedMap) {
    for (const auto &[zgramId, count] : inner) {
      metadata_.plusPlusKeys()[zgramId].insert(key);

      if (count > 0) {
        addEntries(&metadata_.plusPluses(), key, zgramId, count);
      } else if (count < 0) {
        addEntries(&metadata_.minusMinuses(), key, zgramId, -count);
      } else {
        // This is a bit of a hack. If count == 0 we still want to to dependency tracking.
        // So we add a balanced entry to both sides. Technically we would only need to do
        // this if both plusPluses and minusMinuses are empty, but we don't bother with that
        // optimization
        addEntries(&metadata_.plusPluses(), key, zgramId, 1);
        addEntries(&metadata_.minusMinuses(), key, zgramId, 1);
      }
    }
  }
}

std::ostream &operator<<(std::ostream &s, const DynamicIndex &o) {
  return streamf(s,
      "{trie: %o"
      "\nzgramInfos: %o"
      "\nwordInfos: %o"
      "\nmetadata: %o}",
      o.trie_, o.zgramInfos_, o.wordInfos_, o.metadata_);
}
}  // namespace z2kplus::backend::reverse_index::index
