// Copyright 2023 Corey Kosak
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

#include <limits>

#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"

namespace kosak::coding::text {
namespace internal {
bool tryParseSigned(std::string_view sr, intmax_t minValue, intmax_t maxValue,
  intmax_t *result, std::string_view  *residual, const kosak::coding::FailFrame &ff);
bool tryParseUnsigned(std::string_view sr, size_t maxValue, uintmax_t *result,
  std::string_view *residual, const kosak::coding::FailFrame &ff);
}  // namespace internal

#define KOSAK_CODING_DEFINE_TRY_PARSE_SIGNED(TYPE) \
inline bool tryParseDecimal(std::string_view sv, TYPE *result, std::string_view *residual, \
  const kosak::coding::FailFrame &ff) { \
  intmax_t temp; \
  if (!internal::tryParseSigned(sv, std::numeric_limits<TYPE>::min(), \
    std::numeric_limits<TYPE>::max(), &temp, residual, ff.nest(KOSAK_CODING_HERE))) { \
    return false; \
  } \
  *result = (TYPE)temp; \
  return true; \
}

#define KOSAK_CODING_DEFINE_TRY_PARSE_UNSIGNED(TYPE) \
inline bool tryParseDecimal(std::string_view sr, TYPE *result, std::string_view *residual, \
  const kosak::coding::FailFrame &ff) { \
  uintmax_t temp; \
  if (!internal::tryParseUnsigned(sr, std::numeric_limits<TYPE>::max(), &temp, residual, \
    ff.nest(KOSAK_CODING_HERE))) { \
    return false; \
  } \
  *result = (TYPE)temp; \
  return true; \
}

KOSAK_CODING_DEFINE_TRY_PARSE_SIGNED(int8_t);
KOSAK_CODING_DEFINE_TRY_PARSE_SIGNED(int16_t);
KOSAK_CODING_DEFINE_TRY_PARSE_SIGNED(int32_t);
KOSAK_CODING_DEFINE_TRY_PARSE_SIGNED(int64_t);

KOSAK_CODING_DEFINE_TRY_PARSE_UNSIGNED(uint8_t);
KOSAK_CODING_DEFINE_TRY_PARSE_UNSIGNED(uint16_t);
KOSAK_CODING_DEFINE_TRY_PARSE_UNSIGNED(uint32_t);
KOSAK_CODING_DEFINE_TRY_PARSE_UNSIGNED(uint64_t);

#undef KOSAK_CODING_DEFINE_TRY_PARSE_UNSIGNED
#undef KOSAK_CODING_DEFINE_TRY_PARSE_SIGNED

bool tryConvertIso88591ToUnicode(char src, char *result1, char *result2,
    const kosak::coding::FailFrame &ff);

class ReusableString8 {
  typedef kosak::coding::FailFrame FailFrame;
public:
  bool tryReset(std::u32string_view sv, const FailFrame &ff);

  bool tryReset(char32_t ch, const kosak::coding::FailFrame &ff) {
    return tryReset(std::u32string_view(&ch, 1), ff.nest(KOSAK_CODING_HERE));
  }

  const std::string &storage() const { return storage_; }

private:
  std::string storage_;
};

class ReusableString32 {
  typedef kosak::coding::FailFrame FailFrame;
public:
  bool tryReset(std::string_view sv, const FailFrame &ff);

  bool tryReset(char ch, const FailFrame &ff) {
    return tryReset(std::string_view(&ch, 1), ff.nest(KOSAK_CODING_HERE));
  }

  const std::u32string &storage() const { return storage_; }

private:
  std::u32string storage_;
};

// Converts UTF8 to UTF32. Appends to the end of *dest. Returns false if there's a conversion problem.
bool tryConvertUtf8ToUtf32(std::string_view src, std::u32string *dest,
    const kosak::coding::FailFrame &ff);

// Converts UTF32 to UTF8. Appends to the end of *dest.
bool tryConvertUtf32ToUtf8(std::u32string_view src, std::string *dest,
    const kosak::coding::FailFrame &ff);
}  // namespace kosak::coding::text

