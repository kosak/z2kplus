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

#include "z2kplus/backend/reverse_index/metadata/frozen_metadata.h"

using z2kplus::backend::shared::ZgramId;
using kosak::coding::limit;

namespace z2kplus::backend::reverse_index::metadata {
FrozenMetadata::FrozenMetadata() = default;
FrozenMetadata::FrozenMetadata(FrozenMetadata &&other) noexcept = default;
FrozenMetadata &FrozenMetadata::operator=(FrozenMetadata &&other) noexcept = default;
FrozenMetadata::~FrozenMetadata() = default;

std::ostream &operator<<(std::ostream &s, const FrozenMetadata &o) {
  size_t maxLen = 1000;
  return streamf(s,
      "reactions=%o\n"
      "reactionCounts=%o\n"
      "zgRevs=%o\n"
      "zmojis=%o\n"
      "plusPluses=%o\n",
      "minusMinuses=%o",
      limit(o.reactions_, maxLen),
      limit(o.reactionCounts_, maxLen),
      limit(o.zgramRevisions_, maxLen),
      limit(o.zmojis_, maxLen),
      limit(o.plusPluses_, maxLen),
      limit(o.minusMinuses_, maxLen));
}
}  // z2kplus::backend::reverse_index::metadata
