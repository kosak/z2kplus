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

#include "z2kplus/backend/util/frozen/frozen_string_pool.h"

namespace z2kplus::backend::util::frozen {
FrozenStringPool::FrozenStringPool() = default;
FrozenStringPool::FrozenStringPool(FrozenStringPool &&) noexcept = default;
FrozenStringPool & FrozenStringPool::operator=(FrozenStringPool &&) noexcept = default;
FrozenStringPool::FrozenStringPool(const char *text, FrozenVector<uint32> endOffsets) :
    text_(text), endOffsets_(std::move(endOffsets)) {}

bool FrozenStringPool::tryFind(std::string_view s, frozenStringRef_t *result) const {
  size_t temp;
  if (!tryFindHelper(s, 0, endOffsets_.size(), &temp)) {
    return false;
  }
  *result = frozenStringRef_t(temp);
  return true;
}

bool FrozenStringPool::tryFindHelper(std::string_view probe, size_t begin, size_t end,
    size_t *result) const {
  while (begin != end) {
    auto mid = (begin + end) / 2;
    auto beginOffset = mid == 0 ? 0 : endOffsets_[mid - 1];
    auto endOffset = endOffsets_[mid];
    auto target = std::string_view(text_.get() + beginOffset, endOffset - beginOffset);
    auto cmp = probe.compare(target);
    if (cmp == 0) {
      *result = mid;
      return true;
    }
    if (cmp < 0) {
      end = mid;
    } else {
      begin = mid + 1;
    }
  }
  return false;
}

std::ostream &operator<<(std::ostream &s, const FrozenStringPool &o) {
  const char *sep = "";
  size_t lastEnd = 0;
  for (size_t end : o.endOffsets_) {
    s << sep << std::string_view(o.text_.get() + lastEnd, end - lastEnd);
    sep = ", ";
    lastEnd = end;
  }
  return s;
}
}  // namespace z2kplus::backend::util::frozen
