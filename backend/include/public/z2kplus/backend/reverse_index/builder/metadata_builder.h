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

#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "z2kplus/backend/reverse_index/builder/canonical_string_processor.h"
#include "z2kplus/backend/reverse_index/builder/common.h"
#include "z2kplus/backend/reverse_index/builder/log_splitter.h"
#include "z2kplus/backend/reverse_index/builder/zgram_digestor.h"
#include "z2kplus/backend/reverse_index/metadata/frozen_metadata.h"
#include "z2kplus/backend/util/frozen/frozen_string_pool.h"

namespace z2kplus::backend::reverse_index::builder {
struct MetadataBuilder {
  typedef z2kplus::backend::util::frozen::FrozenStringPool FrozenStringPool;
  typedef z2kplus::backend::reverse_index::builder::ZgramDigestorResult ZgramDigestorResult;
  typedef z2kplus::backend::reverse_index::metadata::FrozenMetadata FrozenMetadata;
  typedef kosak::coding::FailFrame FailFrame;

  MetadataBuilder() = delete;

  static bool tryMakeMetadata(const LogSplitterResult &lsr, const ZgramDigestorResult &iitr,
      const std::string &tempFile, const FrozenStringPool &stringPool, SimpleAllocator *alloc,
      FrozenMetadata *result, const FailFrame &ff);
};
}  // namespace z2kplus::backend::reverse_index::builder
