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

#include <map>
#include <set>
#include <string>
#include <vector>
#include <ostream>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "z2kplus/backend/files/keys.h"
#include "z2kplus/backend/reverse_index/fields.h"
#include "z2kplus/backend/reverse_index/index/frozen_index.h"
#include "z2kplus/backend/reverse_index/metadata/frozen_metadata.h"
#include "z2kplus/backend/reverse_index/types.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/util/frozen/frozen_map.h"
#include "z2kplus/backend/util/frozen/frozen_set.h"
#include "z2kplus/backend/util/frozen/frozen_string_pool.h"
#include "z2kplus/backend/util/frozen/frozen_vector.h"

namespace z2kplus::backend::reverse_index::metadata {
class DynamicMetadata {
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::reverse_index::index::FrozenIndex FrozenIndex;
  typedef z2kplus::backend::reverse_index::metadata::FrozenMetadata FrozenMetadata;
  typedef z2kplus::backend::shared::ZgramCore ZgramCore;
  typedef z2kplus::backend::shared::ZgramId ZgramId;

  template<typename K, typename V>
  using lessMap_t = std::map<K, V, std::less<>>;

public:
  DynamicMetadata();
  DISALLOW_COPY_AND_ASSIGN(DynamicMetadata);
  DECLARE_MOVE_COPY_AND_ASSIGN(DynamicMetadata);
  ~DynamicMetadata();

  bool tryAddHelper(const FrozenIndex &lhs,
      const z2kplus::backend::shared::zgMetadata::Reaction &o, const FailFrame &ff);
  bool tryAddHelper(const FrozenIndex &lhs,
      const z2kplus::backend::shared::zgMetadata::ZgramRevision &o, const FailFrame &ff);
  bool tryAddHelper(const FrozenIndex &lhs,
      const z2kplus::backend::shared::zgMetadata::ZgramRefersTo &o, const FailFrame &ff);
  bool tryAddHelper(const FrozenIndex &lhs,
      const z2kplus::backend::shared::userMetadata::Zmojis &o, const FailFrame &ff);

  // zgramId -> reaction -> creator -> {add, remove}
  typedef lessMap_t<ZgramId, lessMap_t<std::string, lessMap_t<std::string, bool>>> reactions_t;
  // reaction -> zgramId -> count
  typedef lessMap_t<std::string, std::map<ZgramId, int32_t>> reactionCounts_t;
  // zgramId -> ZgramCore revisions.
  typedef std::map<ZgramId, std::vector<ZgramCore>> zgramRevisions_t;
  // zgramId -> map<ZgramId, valid> referred-to zgrams. Set to false to hide a frozen reference
  typedef std::map<ZgramId, std::map<ZgramId, bool>> zgramRefersTo_t;
  // userId -> zmojis
  typedef lessMap_t<std::string, std::string> zmojis_t;
  // key -> vector<ZgramIds> containing "key++". We can figure out the net ++ by doing a rank
  // operation. The vector needs to remain sorted, so internal changes to it are costly. However the
  // dynamic index is typically small so these changes may not matter. If we care we would need to
  // switch to a more efficient data structure (one that still supports rank).
  typedef lessMap_t<std::string, std::vector<ZgramId>> plusPluses_t;
  typedef plusPluses_t minusMinuses_t;
  typedef std::map<ZgramId, std::set<std::string>> plusPlusKeys_t;

  const reactions_t &reactions() const { return reactions_; }
  const reactionCounts_t &reactionCounts() const { return reactionCounts_; }
  const zgramRevisions_t &zgramRevisions() const { return zgramRevisions_; }
  const zgramRefersTo_t &zgramRefersTo() const { return zgramRefersTo_; }
  const zmojis_t &zmojis() const { return zmojis_; }
  plusPluses_t &plusPluses() { return plusPluses_; }
  const plusPluses_t &plusPluses() const { return plusPluses_; }
  minusMinuses_t &minusMinuses() { return minusMinuses_; }
  const minusMinuses_t &minusMinuses() const { return minusMinuses_; }
  plusPlusKeys_t &plusPlusKeys() { return plusPlusKeys_; }
  const plusPlusKeys_t &plusPlusKeys() const { return plusPlusKeys_; }

private:
  reactions_t reactions_;
  reactionCounts_t reactionCounts_;
  zgramRevisions_t zgramRevisions_;
  zgramRefersTo_t zgramRefersTo_;
  zmojis_t zmojis_;
  plusPluses_t plusPluses_;
  minusMinuses_t minusMinuses_;
  plusPlusKeys_t plusPlusKeys_;

  friend std::ostream &operator<<(std::ostream &s, const DynamicMetadata &o);
  DECLARE_TYPICAL_JSON(DynamicMetadata);
};
}  // namespace z2kplus::backend::reverse_index
