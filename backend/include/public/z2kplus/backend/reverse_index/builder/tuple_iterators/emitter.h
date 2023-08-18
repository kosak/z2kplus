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

namespace z2kplus::backend::reverse_index::builder::tuple_iterators::emitter {
template<size_t Level, typename ...Args>
void emitTupleRecurse(const std::tuple<Args...> &src, char fieldSeparator, std::string *dest) {
  if constexpr (Level != sizeof...(Args)) {
    if constexpr (Level != 0) {
      dest->push_back(fieldSeparator);
    }
    emitItem(std::get<Level>(src), dest);
  }
}

template<typename ...Args>
void emitTuple(const std::tuple<Args...> &src, char fieldSeparator, std::string *dest) {
  emitTupleRecurse<0>(src, fieldSeparator, dest);
}
}   // namespace z2kplus::backend::reverse_index::builder::tuple_iterators::emitter
