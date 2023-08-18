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
#include "z2kplus/backend/files/keys.h"
#include "z2kplus/backend/reverse_index/fields.h"
#include "z2kplus/backend/reverse_index/types.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/util/frozen/frozen_map.h"
#include "z2kplus/backend/util/frozen/frozen_set.h"
#include "z2kplus/backend/util/frozen/frozen_string_pool.h"
#include "z2kplus/backend/util/frozen/frozen_vector.h"

namespace z2kplus::backend::reverse_index::metadata {
class FrozenMetadata {
  typedef z2kplus::backend::files::Location Location;
  typedef z2kplus::backend::shared::RenderStyle RenderStyle;
  typedef z2kplus::backend::shared::ZgramId ZgramId;
  typedef z2kplus::backend::util::frozen::frozenStringRef_t frozenStringRef_t;

  template<typename K, typename V>
  using FrozenMap = z2kplus::backend::util::frozen::FrozenMap<K, V>;
  template<typename T>
  using FrozenSet = z2kplus::backend::util::frozen::FrozenSet<T>;
  template<typename T>
  using FrozenVector = z2kplus::backend::util::frozen::FrozenVector<T>;
  // Let's just assume that std::tuple is blittable
  template<typename ...ARGS>
  using FrozenTuple = std::tuple<ARGS...>;

public:
  // zgramId -> reaction -> {creator}
  typedef FrozenMap<ZgramId, FrozenMap<frozenStringRef_t, FrozenSet<frozenStringRef_t>>> reactions_t;
  // reaction -> zgramId -> count
  typedef FrozenMap<frozenStringRef_t, FrozenMap<ZgramId, int64_t>> reactionCounts_t;
  // zgramId -> [tuple<instance, body, renderstyle>].
  typedef FrozenMap<ZgramId, FrozenVector<FrozenTuple<frozenStringRef_t, frozenStringRef_t, uint32_t>>> zgramRevisions_t;
  // userId -> zmojis
  typedef FrozenMap<frozenStringRef_t, frozenStringRef_t> zmojis_t;
  // key -> vector<ZgramIds> containing "key++". We can figure out the net ++ by doing a rank operation
  typedef FrozenMap<frozenStringRef_t, FrozenVector<ZgramId>> plusPluses_t;
  // key -> vector<ZgramIds> containing "key--". We can figure out the net -- by doing a rank operation
  typedef plusPluses_t minusMinuses_t;
  // zgram -> vector<keys> mentioned in zgram, even if they net to zero or if they are ~~ or ??
  typedef FrozenMap<ZgramId, FrozenVector<frozenStringRef_t>> plusPlusKeys_t;

  FrozenMetadata();

  FrozenMetadata(reactions_t reactions, reactionCounts_t reactionCounts, zgramRevisions_t zgramRevisions,
      zmojis_t zmojis, plusPluses_t plusPluses, minusMinuses_t minusminuses, plusPlusKeys_t plusPlusKeys) :
      reactions_(std::move(reactions)), reactionCounts_(std::move(reactionCounts)),
      zgramRevisions_(std::move(zgramRevisions)), zmojis_(std::move(zmojis)),
      plusPluses_(std::move(plusPluses)), minusMinuses_(std::move(minusminuses)),
      plusPlusKeys_(std::move(plusPlusKeys)) {}
  DISALLOW_COPY_AND_ASSIGN(FrozenMetadata);
  DECLARE_MOVE_COPY_AND_ASSIGN(FrozenMetadata);
  ~FrozenMetadata();

  const reactions_t &reactions() const { return reactions_; }
  const reactionCounts_t &reactionCounts() const { return reactionCounts_; }
  const zgramRevisions_t &zgramRevisions() const { return zgramRevisions_; }
  const zmojis_t &zmojis() const { return zmojis_; }
  const plusPluses_t &plusPluses() const { return plusPluses_; }
  const minusMinuses_t &minusMinuses() const { return minusMinuses_; }
  const plusPlusKeys_t &plusPlusKeys() const { return plusPlusKeys_; }

private:
  reactions_t reactions_;
  reactionCounts_t reactionCounts_;
  zgramRevisions_t zgramRevisions_;
  zmojis_t zmojis_;
  plusPluses_t plusPluses_;
  minusMinuses_t minusMinuses_;
  plusPlusKeys_t plusPlusKeys_;

  friend std::ostream &operator<<(std::ostream &s, const FrozenMetadata &o);
};
}  // namespace z2kplus::backend::reverse_index::metadata

