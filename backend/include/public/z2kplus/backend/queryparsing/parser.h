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
#include <memory>
#include <string>
#include <string_view>
#include "kosak/coding/failures.h"
#include "z2kplus/backend/reverse_index/iterators/iterator_common.h"

namespace z2kplus::backend::queryparsing {
bool parse(std::string_view text, bool emptyMeansEverything,
  std::unique_ptr<reverse_index::iterators::ZgramIterator> *result, const kosak::coding::FailFrame &ff);
}  // namespace z2kplus::backend::queryparsing
