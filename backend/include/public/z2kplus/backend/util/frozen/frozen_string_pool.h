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
#include <cstdlib>
#include <ostream>
#include <string_view>
#include "kosak/coding/coding.h"
#include "kosak/coding/comparers.h"
#include "kosak/coding/strongint.h"
#include "z2kplus/backend/util/relative.h"
#include "z2kplus/backend/util/frozen/frozen_vector.h"

namespace z2kplus::backend::util::frozen {
namespace internal {
struct FrozenStringTag {
  static constexpr const char text[] = "FrozenString";
};
}  // namespace internal
typedef kosak::coding::StrongInt<uint32_t, internal::FrozenStringTag> frozenStringRef_t;
class FrozenStringPool {
public:
  FrozenStringPool();
  DISALLOW_COPY_AND_ASSIGN(FrozenStringPool);
  DECLARE_MOVE_COPY_AND_ASSIGN(FrozenStringPool);
  FrozenStringPool(const char *text, FrozenVector<uint32_t> endOffsets);
  ~FrozenStringPool() = default;

  std::string_view toStringView(frozenStringRef_t stringRef) const {
    auto raw = stringRef.raw();
    auto beginOff = raw == 0 ? 0 : endOffsets_[raw - 1];
    auto endOff = endOffsets_[raw];
    return std::string_view(text_.get() + beginOff, endOff - beginOff);
  }

  bool tryFind(std::string_view s, frozenStringRef_t *result) const;

  size_t size() const {
    return endOffsets_.size();
  }

private:
  bool tryFindHelper(std::string_view probe, size_t begin, size_t end, size_t *result) const;

  RelativePtr<const char> text_;
  FrozenVector<uint32_t> endOffsets_;

  friend std::ostream &operator<<(std::ostream &s, const FrozenStringPool &o);
};
}  // namespace z2kplus::backend::util::frozen
