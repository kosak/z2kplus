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
#include "z2kplus/backend/files/keys.h"
#include "z2kplus/backend/reverse_index/fields.h"
#include "z2kplus/backend/reverse_index/metadata/frozen_metadata.h"
#include "z2kplus/backend/reverse_index/types.h"
#include "z2kplus/backend/reverse_index/trie/frozen_trie.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/util/frozen/frozen_string_pool.h"

namespace z2kplus::backend::reverse_index::index {
class FrozenLess {
  typedef z2kplus::backend::util::frozen::FrozenStringPool FrozenStringPool;
  typedef z2kplus::backend::util::frozen::frozenStringRef_t frozenStringRef_t;

public:
  explicit FrozenLess(const FrozenStringPool *pool) : pool_(pool) {}

  bool operator()(frozenStringRef_t lhs, frozenStringRef_t rhs) const {
    // This works because the frozen strings are ordered.
    return lhs < rhs;
  }
  bool operator()(frozenStringRef_t lhs, std::string_view rhs) const {
    return pool_->toStringView(lhs) < rhs;
  }
  bool operator()(std::string_view lhs, frozenStringRef_t rhs) const {
    return lhs < pool_->toStringView(rhs);
  }
private:
  // Does not own.
  const FrozenStringPool *pool_ = nullptr;
};

class FrozenIndex {
  typedef z2kplus::backend::reverse_index::trie::FrozenTrie FrozenTrie;
  typedef z2kplus::backend::reverse_index::metadata::FrozenMetadata FrozenMetadata;
  typedef z2kplus::backend::util::frozen::FrozenStringPool FrozenStringPool;
  template<typename T>
  using FrozenVector = z2kplus::backend::util::frozen::FrozenVector<T>;
  template<typename K, typename V>
  using FrozenMap = z2kplus::backend::util::frozen::FrozenMap<K, V>;
  template<bool IsLogged>
  using FilePosition = z2kplus::backend::files::FilePosition<IsLogged>;

public:
  FrozenIndex();
  FrozenIndex(const FilePosition<true> &loggedEnd, const FilePosition<false> &unloggedEnd,
      FrozenVector<ZgramInfo> zgramInfos, FrozenVector<WordInfo> wordInfos, FrozenTrie trie,
      FrozenStringPool stringPool, FrozenMetadata metadata);
  DISALLOW_COPY_AND_ASSIGN(FrozenIndex);
  DECLARE_MOVE_COPY_AND_ASSIGN(FrozenIndex);
  ~FrozenIndex();

  FrozenLess makeLess() const {
    return FrozenLess(&stringPool_);
  }

  const FilePosition<true> &loggedEnd() const { return loggedEnd_; }
  const FilePosition<false> &unloggedEnd() const { return unloggedEnd_; }
  const FrozenVector<ZgramInfo> &zgramInfos() const { return zgramInfos_; }
  const FrozenVector<WordInfo> &wordInfos() const { return wordInfos_; }
  const FrozenTrie &trie() const { return trie_; }

  const FrozenStringPool &stringPool() const { return stringPool_; }

  FrozenMetadata &metadata() { return metadata_; }
  const FrozenMetadata &metadata() const { return metadata_; }

private:
  FilePosition<true> loggedEnd_;
  FilePosition<false> unloggedEnd_;
  FrozenVector<ZgramInfo> zgramInfos_;
  FrozenVector<WordInfo> wordInfos_;
  FrozenTrie trie_;
  FrozenStringPool stringPool_;
  FrozenMetadata metadata_;

  friend std::ostream &operator<<(std::ostream &s, const FrozenIndex &o);
};
}   // namespace z2kplus::backend::reverse_index::index
