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
#include "kosak/coding/text/misc.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/files/keys.h"

namespace z2kplus::backend::reverse_index::builder::tuple_iterators::tupleSerializer {
namespace internal {
struct TupleItemSerializers {
  typedef kosak::coding::FailFrame FailFrame;
  typedef kosak::coding::text::Splitter Splitter;
  typedef z2kplus::backend::shared::ZgramId ZgramId;
  typedef z2kplus::backend::files::FileKeyKind FileKeyKind;
  template<FileKeyKind Kind>
  using FileKey = z2kplus::backend::files::FileKey<Kind>;

  static bool tryAppendItem(const bool *src, std::string *dest, const FailFrame &ff);
  static bool tryAppendItem(const uint32_t *src, std::string *dest, const FailFrame &ff);
  static bool tryAppendItem(const uint64_t *src, std::string *dest, const FailFrame &ff);
  static bool tryAppendItem(const std::string_view *src, std::string *dest, const FailFrame &ff);
  static bool tryAppendItem(const ZgramId *src, std::string *dest, const FailFrame &ff);
  static bool tryAppendItem(const FileKey<FileKeyKind::Either> *src, std::string *dest, const FailFrame &ff);

  static bool tryParseItem(std::string_view src, bool *dest, const FailFrame &ff);
  static bool tryParseItem(std::string_view src, uint32_t *dest, const FailFrame &ff);
  static bool tryParseItem(std::string_view src, int64_t *dest, const FailFrame &ff);
  static bool tryParseItem(std::string_view src, uint64_t *dest, const FailFrame &ff);
  static bool tryParseItem(std::string_view src, std::string_view *dest, const FailFrame &ff);
  static bool tryParseItem(std::string_view src, ZgramId *dest, const FailFrame &ff);
  static bool tryParseItem(std::string_view src, FileKey<FileKeyKind::Either> *dest, const FailFrame &ff);

  template<size_t Level, typename ...Args>
  static bool tryAppendTupleRecurse(const std::tuple<Args...> &src, char fieldSeparator,
      std::string *dest, const FailFrame &ff) {
    if constexpr (Level != sizeof...(Args)) {
      if constexpr (Level != 0) {
        dest->push_back(fieldSeparator);
      }
      return TupleItemSerializers::tryAppendItem(&std::get<Level>(src), dest, ff.nest(KOSAK_CODING_HERE)) &&
          tryAppendTupleRecurse<Level + 1>(src, fieldSeparator, dest, ff.nest(KOSAK_CODING_HERE));
    }
    return true;
  }

  template<size_t Level, typename ...Args>
  static bool tryParseTupleRecurse(Splitter *splitter, std::tuple<Args...> *dest,
      const FailFrame &ff) {
    if constexpr (Level == sizeof...(Args)) {
      return splitter->tryConfirmEmpty(ff.nest(KOSAK_CODING_HERE));
    } else {
      std::string_view field;
      return splitter->tryMoveNext(&field, ff.nest(KOSAK_CODING_HERE)) &&
          TupleItemSerializers::tryParseItem(field, &std::get<Level>(*dest), ff.nest(KOSAK_CODING_HERE)) &&
          tryParseTupleRecurse<Level + 1>(splitter, dest, ff.nest(KOSAK_CODING_HERE));
    }
  }
};
}  // namespace internal

template<typename ...Args>
bool tryAppendTuple(const std::tuple<Args...> &src, char fieldSeparator, std::string *dest,
    const kosak::coding::FailFrame &ff) {
  static_assert(sizeof...(Args) != 0);
  return internal::TupleItemSerializers::tryAppendTupleRecurse<0>(src, fieldSeparator, dest,
      ff.nest(KOSAK_CODING_HERE));
}

template<typename ...Args>
bool tryParseTuple(std::string_view record, char fieldSeparator,
    std::tuple<Args...> *dest, const kosak::coding::FailFrame &ff) {
  static_assert(sizeof...(Args) != 0);
  auto splitter = kosak::coding::text::Splitter::of(record, fieldSeparator);
  return internal::TupleItemSerializers::tryParseTupleRecurse<0>(&splitter, dest,
      ff.nest(KOSAK_CODING_HERE));
}
}   // namespace z2kplus::backend::reverse_index::builder::tuple_iterators::tupleSerializer
