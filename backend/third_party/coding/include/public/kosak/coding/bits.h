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

#include "kosak/coding/coding.h"

namespace kosak::coding {
namespace bits {

inline size_t popcount(unsigned int bitset) {
  static_assert(__SSE4_2__, "For best performance, you should compile this code with -msse4.2 or better");
  return __builtin_popcount(bitset);
}

inline size_t popcount(unsigned long bitset) {
  static_assert(__SSE4_2__, "For best performance, you should compile this code with -msse4.2 or better");
  return __builtin_popcountl(bitset);
}

inline size_t popcount(unsigned long long bitset) {
  static_assert(__SSE4_2__, "For best performance, you should compile this code with -msse4.2 or better");
  return __builtin_popcountll(bitset);
}

inline size_t findFirstSet(unsigned int bitset) {
  myassert(bitset != 0);
  return __builtin_ctz(bitset);
}

inline size_t findFirstSet(unsigned long bitset) {
  myassert(bitset != 0);
  return __builtin_ctzl(bitset);
}

inline size_t findFirstSet(unsigned long long bitset) {
  myassert(bitset != 0);
  return __builtin_ctzll(bitset);
}

// Removes the lowest bit from 'bitSet' and returns its index. If *bitSet is all-zero, the results
// are undefined.
inline size_t removeLowestBit(unsigned int *bitSet) {
  myassert(*bitSet != 0);
  auto result = findFirstSet(*bitSet);
  *bitSet &= *bitSet - 1;
  return result;
}

// Removes the lowest bit from 'bitSet' and returns its index. If *bitSet is all-zero, the results
// are undefined.
inline size_t removeLowestBit(unsigned long *bitSet) {
  myassert(*bitSet != 0);
  auto result = findFirstSet(*bitSet);
  *bitSet &= *bitSet - 1;
  return result;
}

// Removes the lowest bit from 'bitSet' and returns its index. If *bitSet is all-zero, the results
// are undefined.
inline size_t removeLowestBit(unsigned long long *bitSet) {
  myassert(*bitSet != 0);
  auto result = findFirstSet(*bitSet);
  *bitSet &= *bitSet - 1;
  return result;
}

inline bool tryRemoveLowestBit(unsigned int *set, unsigned int *result) {
  if (*set == 0) {
    return false;
  }
  *result = findFirstSet(*set);
  *set &= (*set - 1);
  return true;
}
}  // namespace bits
}  // namespace kosak::coding
