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

#include "z2kplus/backend/reverse_index/builder/inflator.h"

using kosak::coding::FailFrame;
#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::reverse_index::builder::inflator {
namespace internal {
bool InflatorBase::tryConfirmHasValue(bool hasValue, const FailFrame &ff) {
  if (!hasValue) {
    return ff.fail(HERE, "Item was required to have a value");
  }
  return true;
}

bool InflatorBase::tryGetNextSize(size_t level, uint64_t *size, const FailFrame &ff) {
  std::optional<uint64_t> osize;
  if (!counts_->tryGetNext(&osize, ff.nest(HERE))) {
    return false;
  }
  if (!osize.has_value()) {
    return ff.fail(HERE, "Underlying iterator exhausted!");
  }
  *size = *osize;
  return true;
}
}  // namespace internal
}  // namespace z2kplus::backend::reverse_index::builder::inflator
