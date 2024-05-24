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
#define KOSAK_CREATE_COMPARER(TYPE) \
constexpr inline int compare(const TYPE *lhs, const TYPE *rhs) { \
  if (*lhs < *rhs) { \
    return -1; \
  } \
  if (*rhs < *lhs) { \
    return 1; \
  } \
  return 0; \
}

KOSAK_CREATE_COMPARER(char);

KOSAK_CREATE_COMPARER(signed char);
KOSAK_CREATE_COMPARER(short);
KOSAK_CREATE_COMPARER(int);
KOSAK_CREATE_COMPARER(long);
KOSAK_CREATE_COMPARER(long long);

KOSAK_CREATE_COMPARER(unsigned char);
KOSAK_CREATE_COMPARER(unsigned short);
KOSAK_CREATE_COMPARER(unsigned int);
KOSAK_CREATE_COMPARER(unsigned long);
KOSAK_CREATE_COMPARER(unsigned long long);

KOSAK_CREATE_COMPARER(bool);
#undef CREATE_COMPARER

}  // namespace kosak::coding
