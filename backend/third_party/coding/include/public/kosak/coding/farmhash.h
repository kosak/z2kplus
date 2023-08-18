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

// Adapted/Copied from Geoff Pike's FarmHash.

#include <cstdlib>
#include "kosak/coding/coding.h"

namespace kosak::coding {

class FarmHash {
public:
  static uint64 HashLen32(const char *s) {
    const size_t len = 32;
    uint64 mul = k2 + len * 2;
    uint64 a = Fetch64(s) * k1;
    uint64 b = Fetch64(s + 8);
    uint64 c = Fetch64(s + len - 8) * mul;
    uint64 d = Fetch64(s + len - 16) * k2;
    return HashLen16(Rotate(a + b, 43) + Rotate(c, 30) + d,
        a + Rotate(b + k2, 18) + c, mul);
  }

  static uint64 HashLen16(const char *s) {
    const uint64 kMul = 0x9ddfea08eb382d69ULL;
    uint64 low;
    uint64 high;
    memcpy(&low, s, sizeof(low));
    memcpy(&high, s + sizeof(low), sizeof(low));
    return HashLen16(low, high, kMul);
  }

private:
  static uint64 Fetch64(const char *p) {
    uint64 result;
    memcpy(&result, p, sizeof(result));
    return result;
  }

  static uint64 Rotate(uint64 val, int shift) {
    // Avoid shifting by 64: doing so yields an undefined result.
    return shift == 0 ? val : ((val >> shift) | (val << (64 - shift)));
  }

  static uint64 HashLen16(uint64 u, uint64 v, uint64 mul) {
    // Murmur-inspired hashing.
    uint64 a = (u ^ v) * mul;
    a ^= (a >> 47);
    uint64 b = (v ^ a) * mul;
    b ^= (b >> 47);
    b *= mul;
    return b;
  }

  // Some primes between 2^63 and 2^64 for various uses.
  static constexpr uint64_t k0 = 0xc3a5c85c97cb3127ULL;
  static constexpr uint64_t k1 = 0xb492b66fbe98f273ULL;
  static constexpr uint64_t k2 = 0x9ae16a3b2f90404fULL;
};
}  // namespace kosak::coding
