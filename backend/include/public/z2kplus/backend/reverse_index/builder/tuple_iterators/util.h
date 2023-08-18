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

#include <array>
#include <tuple>
#include <cstdlib>
#include "z2kplus/backend/util/frozen/frozen_string_pool.h"

namespace z2kplus::backend::reverse_index::builder::tuple_iterators {
namespace traits {
template<typename, typename>
struct ConcatTuple {};

template<typename... Ts, typename... Us>
struct ConcatTuple<std::tuple<Ts...>, std::tuple<Us...>> {
  typedef std::tuple<Ts..., Us...> type;
};

template<size_t, class>
struct TruncateTuple;

template<class... Args>
struct TruncateTuple<0, std::tuple<Args...>> {
  using type = std::tuple<>;
};

template<typename T, class... Args>
struct TruncateTuple<0, std::tuple<T, Args...>> {
  using type = std::tuple<>;
};

template<size_t Count, typename T, typename... Args>
struct TruncateTuple<Count, std::tuple<T, Args...>> {
  using rest = typename TruncateTuple<Count - 1, std::tuple<Args...>>::type;
  using type = typename ConcatTuple<std::tuple<T>, rest>::type;
};

template<typename T>
struct FreezeDimensionsHelper {
  typedef T type;
};
template<>
struct FreezeDimensionsHelper<std::string_view> {
  typedef z2kplus::backend::util::frozen::frozenStringRef_t type;
};

template<typename T>
struct FreezeDimensions;

template<class... Args>
struct FreezeDimensions<std::tuple<Args...>> {
  typedef std::tuple<
      typename FreezeDimensionsHelper<Args>::type...
  > type;
};
}  // namespace traits

template<size_t Begin, size_t End, typename Left, typename Right>
inline size_t findFirstDifference(const Left &lhs, const Right &rhs) {
  if constexpr (Begin == End) {
    return End;
  } else {
    if (std::get<Begin>(lhs) != std::get<Begin>(rhs)) {
      return Begin;
    }
    return findFirstDifference<Begin + 1, End, Left, Right>(lhs, rhs);
  }
}

template<size_t Begin, size_t End, typename Source, typename Dest>
void copySegment(const Source &src, Dest *dest) {
  if constexpr (Begin != End) {
    std::get<Begin>(*dest) = std::get<Begin>(src);
    copySegment<Begin + 1, End, Source, Dest>(src, dest);
  }
}


template<size_t Level, typename Tuple>
bool tryFreezeTupleRecurse(Tuple &src, typename traits::FreezeDimensions<Tuple>::type *dest,
    const z2kplus::backend::util::frozen::FrozenStringPool *stringPool) {
  if constexpr (Level < std::tuple_size_v<Tuple>) {
    const auto &itemSrc = std::get<Level>(src);
    auto &itemDest = std::get<Level>(*dest);
    using strippedType = std::remove_cv_t<std::remove_reference_t<decltype(itemSrc)>>;
    if constexpr (std::is_same_v<strippedType, std::string_view>) {
      if (!stringPool->tryFind(itemSrc, &itemDest)) {
        return false;
      }
    } else {
      itemDest = itemSrc;
    }
    return tryFreezeTupleRecurse<Level + 1>(src, dest, stringPool);
  }
  return true;
}
}  // namespace z2kplus::backend::reverse_index::builder::tuple_iterators
