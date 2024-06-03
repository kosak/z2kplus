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

#include "z2kplus/backend/reverse_index/builder/tuple_iterators/tuple_serializer.h"

#include "kosak/coding/mystream.h"
#include "kosak/coding/text/conversions.h"

using kosak::coding::FailFrame;
using kosak::coding::MyOstringStream;
using kosak::coding::text::tryParseDecimal;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::reverse_index::builder::tuple_iterators::tupleSerializer {
namespace internal {
bool TupleItemSerializers::tryAppendItem(const bool *src, std::string *dest, const FailFrame &) {
  dest->push_back(*src ? 'T' : 'F');
  return true;
}

bool TupleItemSerializers::tryAppendItem(const uint32 *src, std::string *dest, const FailFrame &) {
  MyOstringStream oss(dest);
  oss << *src;
  return true;
}

bool TupleItemSerializers::tryAppendItem(const uint64 *src, std::string *dest, const FailFrame &) {
  MyOstringStream oss(dest);
  oss << *src;
  return true;
}

bool TupleItemSerializers::tryAppendItem(const std::string_view *src, std::string *dest, const FailFrame &) {
  dest->append(*src);
  return true;
}

bool TupleItemSerializers::tryAppendItem(const ZgramId *src, std::string *dest, const FailFrame &ff) {
  auto raw = src->raw();
  return tryAppendItem(&raw, dest, ff.nest(HERE));
}

bool TupleItemSerializers::tryAppendItem(const CompressedFileKey *src, std::string *dest, const FailFrame &ff) {
  auto raw = src->raw();
  return tryAppendItem(&raw, dest, ff.nest(HERE));
}

bool TupleItemSerializers::tryParseItem(std::string_view src, bool *dest, const FailFrame &ff) {
  if (src.size() != 1 || (src[0] != 'T' && src[0] != 'F')) {
    return ff.failf(HERE, R"(Expected "T" or "F", got %o)", src);
  }
  *dest = src[0] == 'T';
  return true;
}

bool TupleItemSerializers::tryParseItem(std::string_view src, uint32_t *dest, const FailFrame &ff) {
  return tryParseDecimal(src, dest, nullptr, ff.nest(HERE));
}
bool TupleItemSerializers::tryParseItem(std::string_view src, int64_t *dest, const FailFrame &ff) {
  return tryParseDecimal(src, dest, nullptr, ff.nest(HERE));
}
bool TupleItemSerializers::tryParseItem(std::string_view src, uint64_t *dest, const FailFrame &ff) {
  return tryParseDecimal(src, dest, nullptr, ff.nest(HERE));
}
bool TupleItemSerializers::tryParseItem(std::string_view src, std::string_view *dest, const FailFrame &) {
  *dest = src;
  return true;
}
bool TupleItemSerializers::tryParseItem(std::string_view src, ZgramId *dest, const FailFrame &ff) {
  decltype(dest->raw()) raw;
  if (!tryParseItem(src, &raw, ff.nest(HERE))) {
    return false;
  }
  *dest = ZgramId(raw);
  return true;
}

bool TupleItemSerializers::tryParseItem(std::string_view src, CompressedFileKey *dest, const FailFrame &ff) {
  decltype(dest->raw()) raw;
  if (!tryParseItem(src, &raw, ff.nest(HERE))) {
    return false;
  }
  *dest = CompressedFileKey(raw);
  return true;
}
}  // namespace internal
}  // namespace z2kplus::backend::reverse_index::builder::tuple_iterators::tupleSerializer
