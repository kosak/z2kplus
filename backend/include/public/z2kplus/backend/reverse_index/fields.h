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

#include <string>
#include <string_view>
#include "kosak/coding/coding.h"

namespace z2kplus::backend::reverse_index {

enum class FieldTag {
  sender, signature, instance, body, numTags
};
const char *getName(FieldTag tag);
bool tryParse(std::string_view text, FieldTag *result);
inline std::ostream &operator<<(std::ostream &s, FieldTag tag) { return s << getName(tag); }

enum class FieldMask {
  none = 0,
  sender = 1U << (unsigned)FieldTag::sender,
  signature = 1U << (unsigned)FieldTag::signature,
  instance = 1U << (unsigned)FieldTag::instance,
  body = 1U << (unsigned)FieldTag::body,
  deflt = (unsigned)sender | (unsigned)instance | (unsigned)body,
  all = (unsigned)sender | (unsigned)signature | (unsigned)instance | (unsigned)body
};

std::ostream &operator<<(std::ostream &s, FieldMask mask);

}  // namespace z2kplus::backend::reverse_index
