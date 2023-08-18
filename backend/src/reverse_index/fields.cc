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

#include "z2kplus/backend/reverse_index/fields.h"

#include <string>
#include <string_view>
#include "kosak/coding/coding.h"
#include "kosak/coding/bits.h"

namespace z2kplus::backend::reverse_index {
using kosak::coding::bits::tryRemoveLowestBit;

namespace {
std::array<const char *, (int)FieldTag::numTags> fieldTagNames = {
    "sender",
    "signature",
    "instance",
    "body"
};

static_assert((int) FieldTag::sender == 0);
static_assert((int) FieldTag::signature == 1);
static_assert((int) FieldTag::instance == 2);
static_assert((int) FieldTag::body == 3);
static_assert((int) FieldTag::numTags == 4);
}  // namespace

const char *getName(FieldTag tag) {
  passert(tag < FieldTag::numTags);
  return fieldTagNames[(int)tag];
}

bool tryParse(std::string_view text, FieldTag *result) {
  for (size_t i = 0; i < fieldTagNames.size(); ++i) {
    if (strncmp(text.data(), fieldTagNames[i], text.size()) == 0) {
      *result = (FieldTag)i;
      return true;
    }
  }
  return false;
}

std::ostream &operator<<(std::ostream &s, FieldMask mask) {
  switch (mask) {
    case FieldMask::none:
      return s << "(none)";
    case FieldMask::deflt:
      return s << "(default)";
    case FieldMask::all:
      return s << "ALL";
    default: {
      auto value = (uint32_t)mask;
      uint32_t nextBit;
      const char *separator = "";
      while (tryRemoveLowestBit(&value, &nextBit)) {
        s << separator << (FieldTag) nextBit;
        separator = "|";
      }
      return s;
    }
  }
}

} // namespace z2kplus::backend::reverse_index
