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

#include "kosak/coding/text/conversions.h"

#include <string_view>
#include "kosak/coding/coding.h"
#include "kosak/coding/dumping.h"

using kosak::coding::Hexer;
using kosak::coding::FailFrame;

#define HERE KOSAK_CODING_HERE

namespace kosak::coding::text {
bool tryConvertIso88591ToUnicode(char ch, char *result1, char *result2, const FailFrame &ff) {
  auto uch = bit_cast<unsigned char>(ch);
  if (uch < 0x80) {
    return ff.failf(HERE, "%o is not a special ISO8859-1 character that I am prepared to convert", ch);
  }
  *result1 = (char)(0xC0U | (unsigned char)(uch >> 6U));
  *result2 = (char)(0x80U | (uch & 0x3fU));
  return true;
}

namespace {
bool tryParseUnsignedNoPrefix(std::string_view sr, uintmax_t maxVal, uintmax_t *result,
  std::string_view *residual, const FailFrame &ff) {
  *result = 0;
  size_t i = 0;
  bool valid = false;
  for (; i < sr.size(); ++i) {
    char ch = sr[i];
    if (ch < '0' || ch > '9') {
      break;
    }
    *result = *result * 10 + (ch - '0');
    valid = true;
  }

  if (!valid) {
    return ff.failf(HERE, R"(Input "%o" does not have a decimal prefix)", sr);
  }

  if (*result > maxVal) {
    return ff.failf(HERE, R"(Input "%o" larger than allowable max %o)", *result, maxVal);
  }

  // If caller does not specify space for the residual, then all the input must be consumed.
  if (residual != nullptr) {
    *residual = sr.substr(i);
  } else if (sr.size() != i) {
    return ff.failf(HERE, R"(Trailing nondecimal text "%o")", sr.substr(i));
  }
  return true;
}
}  // namespace

namespace internal {
bool tryParseSigned(std::string_view sr, intmax_t minValue, intmax_t maxValue, intmax_t *result,
  std::string_view *residual, const FailFrame &ff) {
  size_t i = 0;
  int sign = 1;
  if (!sr.empty()) {
    char first = sr[0];
    if (first == '+') {
      ++i;
    } else if (first == '-') {
      sign = -1;
      ++i;
    }
  }
  uintmax_t uResult;
  if (!tryParseUnsignedNoPrefix(sr.substr(i), maxValue, &uResult, residual, ff.nest(HERE))) {
    return false;
  }
  *result = static_cast<intmax_t>(uResult) * sign;
  if (*result < minValue) {
    return ff.failf(HERE, R"(Input "%o" smaller than allowable min %o)", *result, minValue);
  }
  return true;
}

bool tryParseUnsigned(std::string_view sr, size_t maxValue, uintmax_t *result,
  std::string_view *residual, const FailFrame &ff) {
  size_t i = 0;
  if (!sr.empty() && sr[0] == '+') {
    ++i;
  }
  return tryParseUnsignedNoPrefix(sr.substr(i), maxValue, result, residual, ff.nest(HERE));
}
}  // namespace internal

bool ReusableString8::tryReset(std::u32string_view src, const FailFrame &ff) {
  storage_.clear();
  return tryConvertUtf32ToUtf8(src, &storage_, ff.nest(HERE));
}

bool ReusableString32::tryReset(std::string_view src, const FailFrame &ff) {
  storage_.clear();
  return tryConvertUtf8ToUtf32(src, &storage_, ff.nest(HERE));
}

bool tryConvertUtf8ToUtf32(std::string_view src, std::u32string *dest, const FailFrame &ff) {
  for (size_t i = 0; i < src.size(); ++i) {
    auto uch = bit_cast<unsigned char>(src[i]);
    if ((uch & 0b1'0000000U) == 0b0'0000000U) {
      dest->push_back(uch);
      continue;
    }
    char32_t build;
    size_t residualNeeded;
    if ((uch & 0b111'00000U) == 0b110'00000U) {
      build = uch & 0b000'11111U;
      residualNeeded = 1;
    } else if ((uch & 0b1111'0000U) == 0b1110'0000U) {
      build = uch & 0b0000'1111U;
      residualNeeded = 2;
    } else if ((uch & 0b11111'000U) == 0b11110'000U) {
      residualNeeded = 3;
      build = uch & 0b00000'111U;
    } else {
      return ff.failf(HERE, "Bad start of UTF-8 sequence 0x%o", Hexer(uch));
    }

    while (residualNeeded-- != 0) {
      uch = bit_cast<unsigned char>(src[++i]);
      if ((uch & 0b11'000000U) != 0b10'000000U) {
        return ff.failf(HERE, "Expected 10xxxxxx character in UTF-8 sequence, got 0x%o", Hexer(uch));
      }
      build = (build << 6U) | (uch & 0b00'111111U);
    }
    dest->push_back(build);
  }
  return true;
}

bool tryConvertUtf32ToUtf8(std::u32string_view src, std::string *dest, const FailFrame &ff) {
  auto pushDest = [dest](uint32_t val) {
    dest->push_back(static_cast<char>(val));
  };
  for (auto ch32 : src) {
    if (ch32 <= 0x7f) {
      pushDest(ch32);
      continue;
    }

    if (ch32 <= 0x7ff) {
      auto data0 = (ch32 >> 6U) & 0b000'11111U;
      auto data1 = ch32 & 0b00'111111U;
      pushDest(0b110'00000U | data0);
      pushDest(0b10'000000U | data1);
      continue;
    }

    if (ch32 <= 0xffff) {
      auto data0 = (ch32 >> 12U) & 0b0000'1111U;
      auto data1 = (ch32 >> 6U) & 0b00'111111U;
      auto data2 = ch32 & 0b00'111111U;
      pushDest(0b1110'0000U | data0);
      pushDest(0b10'000000U | data1);
      pushDest(0b10'000000U | data2);
      continue;
    }

    if (ch32 <= 0x10ffff) {
      auto data0 = (ch32 >> 18U) & 0b00000'111U;
      auto data1 = (ch32 >> 12U) & 0b00'111111U;
      auto data2 = (ch32 >> 6U) & 0b00'111111U;
      auto data3 = ch32 & 0b00'111111U;
      pushDest(0b11110'000U | data0);
      pushDest(0b10'000000U | data1);
      pushDest(0b10'000000U | data2);
      pushDest(0b10'000000U | data3);
      continue;
    }

    return ff.failf(HERE, "Bad UTF-32 character 0x%o", Hexer(ch32));
  }
  return true;
}
}  // namespace kosak::coding::text
