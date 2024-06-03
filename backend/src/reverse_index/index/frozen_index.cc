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

#include "z2kplus/backend/reverse_index/index/frozen_index.h"

#include <string>
#include <vector>
#include "kosak/coding/coding.h"

using kosak::coding::streamf;
using z2kplus::backend::shared::Zephyrgram;

namespace z2kplus::backend::reverse_index::index {

FrozenIndex::FrozenIndex() = default;
FrozenIndex::FrozenIndex(const FilePosition<true> &loggedEnd, const FilePosition<false> &unloggedEnd,
    FrozenVector<ZgramInfo> zgramInfos, FrozenVector<WordInfo> wordInfos, FrozenTrie trie,
    FrozenStringPool stringPool, FrozenMetadata metadata) :
    loggedEnd_(loggedEnd), unloggedEnd_(unloggedEnd),
    zgramInfos_(std::move(zgramInfos)),
    wordInfos_(std::move(wordInfos)), trie_(std::move(trie)), stringPool_(std::move(stringPool)),
    metadata_(std::move(metadata)) {}
FrozenIndex::FrozenIndex(FrozenIndex &&other) noexcept = default;
FrozenIndex &FrozenIndex::operator=(FrozenIndex &&other) noexcept = default;
FrozenIndex::~FrozenIndex() = default;

std::ostream &operator<<(std::ostream &s, const FrozenIndex &o) {
  return streamf(s,
    "{loggedEnd: %o"
    "\nunloggedEnd: %o"
    "\ntrie: %o"
    "\nzgramInfos: %o"
    "\nwordInfos: %o"
    "\nstringPool: %o"
    "\nmetadata: %o}",
    o.loggedEnd_, o.unloggedEnd_, o.trie_, o.zgramInfos_, o.wordInfos_, o.stringPool_, o.metadata_);
}
}  // namespace z2kplus::backend::reverse_index::index
