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

#include "kosak/coding/failures.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/reverse_index/builder/log_splitter.h"
#include "z2kplus/backend/reverse_index/trie/frozen_trie.h"
#include "z2kplus/backend/util/frozen/frozen_vector.h"

namespace z2kplus::backend::reverse_index::builder {
class ZgramDigestorResult {
  template<typename T>
  using FrozenVector = z2kplus::backend::util::frozen::FrozenVector<T>;
  typedef z2kplus::backend::reverse_index::trie::FrozenTrie FrozenTrie;

public:
  ZgramDigestorResult();
  ZgramDigestorResult(FrozenVector<ZgramInfo> zgramInfos, FrozenVector<WordInfo> wordInfos,
      FrozenTrie trie, std::string plusPlusEntriesName, std::string minusMinusEntriesName,
      std::string plusPlusKeysName);
  DISALLOW_COPY_AND_ASSIGN(ZgramDigestorResult);
  DECLARE_MOVE_COPY_AND_ASSIGN(ZgramDigestorResult);
  ~ZgramDigestorResult();

  const FrozenVector<ZgramInfo> &zgramInfos() const { return zgramInfos_; };
  FrozenVector<ZgramInfo> &zgramInfos() { return zgramInfos_; };

  const FrozenVector<WordInfo> &wordInfos() const { return wordInfos_; }
  FrozenVector<WordInfo> &wordInfos() { return wordInfos_; }

  const FrozenTrie &trie() const { return trie_; }
  FrozenTrie &trie() { return trie_; }

  const std::string &plusPlusEntriesName() const { return plusPlusEntriesName_; }
  const std::string &minusMinusEntriesName() const { return minusMinusEntriesName_; }
  const std::string &plusPlusKeysName() const { return plusPlusKeysName_; }

private:
  FrozenVector<ZgramInfo> zgramInfos_;
  FrozenVector<WordInfo> wordInfos_;
  FrozenTrie trie_;
  std::string plusPlusEntriesName_;
  std::string minusMinusEntriesName_;
  std::string plusPlusKeysName_;
};

class ZgramDigestor {
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::files::PathMaster PathMaster;

public:
  static bool tryDigest(const PathMaster &pm, const LogSplitterResult &lsr, SimpleAllocator *alloc,
      ZgramDigestorResult *result, const FailFrame &ff);
};
}  // namespace z2kplus::backend::reverse_index::builder
