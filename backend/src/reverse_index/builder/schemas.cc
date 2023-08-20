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

#include "z2kplus/backend/reverse_index/builder/schemas.h"

namespace z2kplus::backend::reverse_index::builder {
namespace schemas {
Zephyrgram::tuple_t Zephyrgram::createTuple(const Zgram &zgram,
    const FileKey &fileKey, size_t offset, size_t size) {
  const auto &zgc = zgram.zgramCore();
  return {
    zgram.zgramId(), zgram.timesecs(),
    zgram.sender(), zgram.signature(), zgram.isLogged(),
    zgc.instance(), zgc.body(),
    fileKey, offset, size
  };
}

auto ReactionsByZgramId::createTuple(const Reaction &reaction) -> tuple_t {
  return {reaction.zgramId(), reaction.reaction(), reaction.creator(), reaction.value()};
}

auto ReactionsByReaction::createTuple(const Reaction &reaction) -> tuple_t {
  return {reaction.reaction(), reaction.zgramId(), reaction.creator(), reaction.value()};
}

auto ZgramRevisions::createTuple(const ZgramRevision &zgRev) -> tuple_t {
  const auto &zgc = zgRev.zgc();
  return {zgRev.zgramId(), zgc.instance(), zgc.body(), (int)zgc.renderStyle()};
}

auto ZgramRefersTos::createTuple(const ZgramRefersTo &refersTo) -> tuple_t {
  return {refersTo.zgramId(), refersTo.refersTo(), refersTo.value()};
}

auto ZmojisRevisions::createTuple(const Zmojis &isLoggedRevision) -> tuple_t {
  return {isLoggedRevision.userId(), isLoggedRevision.zmojis()};
}
}  // namespace schemas
}  // namespace z2kplus::backend::reverse_index::builder
