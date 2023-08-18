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

#include "z2kplus/backend/reverse_index/metadata/dynamic_metadata.h"

#include "kosak/coding/map_utils.h"
#include "z2kplus/backend/reverse_index/index/frozen_index.h"

using kosak::coding::maputils::tryFind;
using z2kplus::backend::reverse_index::index::FrozenIndex;
using z2kplus::backend::shared::ZgramId;
namespace userMetadata = z2kplus::backend::shared::userMetadata;
namespace zgMetadata = z2kplus::backend::shared::zgMetadata;

namespace z2kplus::backend::reverse_index::metadata {
namespace {
bool lookupHelper(const FrozenIndex &frozenIndex, const DynamicMetadata &dynamic,
    const ZgramId &zgramId, const std::string &reaction, const std::string &creator);
}  // namespace

DynamicMetadata::DynamicMetadata() = default;
DynamicMetadata::DynamicMetadata(DynamicMetadata &&other) noexcept = default;
DynamicMetadata &DynamicMetadata::operator=(DynamicMetadata &&other) noexcept = default;
DynamicMetadata::~DynamicMetadata() = default;

bool DynamicMetadata::tryAddHelper(const FrozenIndex &lhs,
    const zgMetadata::Reaction &o, const FailFrame &/*ff*/) {
  auto currentValue = lookupHelper(lhs, *this, o.zgramId(), o.reaction(), o.creator());
  if (o.value() == currentValue) {
    // no change
    return true;
  }
  reactions_[o.zgramId()][o.reaction()][o.creator()] = o.value();
  auto delta = o.value() ? 1 : -1;
  reactionCounts_[o.reaction()][o.zgramId()] += delta;
  return true;
}

bool DynamicMetadata::tryAddHelper(const FrozenIndex &/*lhs*/,
    const zgMetadata::ZgramRevision &o, const FailFrame &/*ff*/) {
  zgramRevisions_[o.zgramId()].push_back(o.zgc());
  return true;
}

bool DynamicMetadata::tryAddHelper(const FrozenIndex &/*lhs*/,
    const userMetadata::Zmojis &o, const FailFrame &/*ff*/) {
  zmojis_.insert_or_assign(o.userId(), o.zmojis());
  return true;
}

std::ostream &operator<<(std::ostream &s, const DynamicMetadata &o) {
  return s << "DynamicMetadata!!!\n";
}

namespace {
bool lookupHelper(const FrozenIndex &frozenIndex, const DynamicMetadata &dynamic,
    const ZgramId &zgramId, const std::string &reaction, const std::string &creator) {
  // If it's in the dynamic metadata, then that value (true or false) dominates
  const DynamicMetadata::reactions_t::mapped_type *dInner1;
  const DynamicMetadata::reactions_t::mapped_type::mapped_type *dInner2;
  const bool *result;
  if (tryFind(dynamic.reactions(), zgramId, &dInner1) &&
    tryFind(*dInner1, reaction, &dInner2) &&
    tryFind(*dInner2, creator, &result)) {
    return *result;
  }

  // Otherwise look to the frozen metadata
  const FrozenMetadata::reactions_t::mapped_type *fInner1;
  const FrozenMetadata::reactions_t::mapped_type::mapped_type *fInner2;
  const FrozenMetadata::reactions_t::mapped_type::mapped_type::value_type *dummy;
  const auto &frozen = frozenIndex.metadata();
  auto less = frozenIndex.makeLess();
  return frozen.reactions().tryFind(zgramId, &fInner1) &&
    fInner1->tryFind(reaction, &fInner2, less) &&
    fInner2->tryFind(creator, &dummy, less);
}
}  // namespace
}  // z2kplus::backend::reverse_index::metadata
