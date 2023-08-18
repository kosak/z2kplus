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

#include <optional>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/text/misc.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/iterator_base.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/tuple_counter.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/util.h"
#include "z2kplus/backend/util/frozen/frozen_map.h"
#include "z2kplus/backend/util/frozen/frozen_set.h"
#include "z2kplus/backend/util/frozen/frozen_vector.h"

namespace z2kplus::backend::reverse_index::builder::inflator {
namespace internal {
class InflatorBase {
protected:
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::reverse_index::builder::tuple_iterators::TupleCounts TupleCounts;
  typedef z2kplus::backend::util::frozen::frozenStringRef_t frozenStringRef_t;

  template<typename T>
  using TupleIterator = z2kplus::backend::reverse_index::builder::tuple_iterators::TupleIterator<T>;
  template<typename K, typename V>
  using FrozenMap = z2kplus::backend::util::frozen::FrozenMap<K, V>;
  template<typename T>
  using FrozenSet = z2kplus::backend::util::frozen::FrozenSet<T>;
  template<typename T>
  using FrozenVector = z2kplus::backend::util::frozen::FrozenVector<T>;

  InflatorBase(SimpleAllocator *alloc, TupleCounts *counts) : alloc_(alloc), counts_(counts) {}

  static bool tryConfirmHasValue(bool hasValue, const FailFrame &ff);

  bool tryGetNextSize(size_t level, uint64_t *size, const FailFrame &ff);

  // Does not own
  SimpleAllocator *alloc_ = nullptr;
  // Does not own
  TupleCounts *counts_ = nullptr;
};

template<typename Tuple>
class Inflator : public InflatorBase {
public:
  Inflator(TupleIterator <Tuple> *iter, SimpleAllocator *alloc, TupleCounts *counts) :
      InflatorBase(alloc, counts), iter_(iter) {}

  template<size_t Level, typename T>
  bool tryInflateRecurse(T *result, const FailFrame &ff);
  template<size_t Level, typename K, typename V>
  bool tryInflateRecurse(std::pair<K, V> *result, const FailFrame &ff);
  template<size_t Level, typename ...ARGS>
  bool tryInflateRecurse(std::tuple<ARGS...> *result, const FailFrame &ff);
  template<size_t Level, typename T>
  bool tryInflateRecurse(FrozenVector<T> *result, const FailFrame &ff);
  template<size_t Level, typename T>
  bool tryInflateRecurse(FrozenSet<T> *result, const FailFrame &ff);
  template<size_t Level, typename K, typename V>
  bool tryInflateRecurse(FrozenMap<K, V> *result, const FailFrame &ff);

  std::optional<Tuple> &current() { return current_; }

private:
  template<size_t SrcLevel, size_t DestLevel, typename ...ARGS>
  bool tryInflateTupleElementRecurse(std::tuple<ARGS...> *result, const FailFrame &ff);

  // Does not own
  TupleIterator<Tuple> *iter_ = nullptr;
  std::optional<Tuple> current_;
};
}  // namespace internal

/**
 * The job of tryInflate is to take a bunch of flattened tuples (provided by the TupleIterator) and "inflate" them
 * to the target data structure (provided by Dest).
 */
template<typename Tuple, typename Dest>
bool tryInflate(
    const std::string &countsName,
    z2kplus::backend::reverse_index::builder::tuple_iterators::TupleIterator<Tuple> *iter,
    size_t treeHeight, Dest *dest, SimpleAllocator *alloc, const kosak::coding::FailFrame &ff) {
  typedef z2kplus::backend::reverse_index::builder::tuple_iterators::TupleCounts TupleCounts;
  while (true) {
    std::optional<Tuple> item;
    if (!iter->tryGetNext(&item, ff.nest(KOSAK_CODING_HERE))) {
      return false;
    }
    if (!item.has_value()) {
      break;
    }
  }
  iter->reset();
  TupleCounts counts;
  if (!TupleCounts::tryCreate(countsName, iter, treeHeight, &counts, ff.nest(KOSAK_CODING_HERE))) {
    return false;
  }
  iter->reset();
  {
    std::optional<uint64_t> item;
    while (true) {
      if (!counts.tryGetNext(&item, ff.nest(KOSAK_CODING_HERE))) {
        return false;
      }
      if (!item.has_value()) {
        counts.reset();
        break;
      }
    }
  }
  internal::Inflator<Tuple> inflator(iter, alloc, &counts);
  // Prime the first value
  if (!iter->tryGetNext(&inflator.current(), ff.nest(KOSAK_CODING_HERE)) ||
      !inflator.template tryInflateRecurse<0>(dest, ff.nest(KOSAK_CODING_HERE))) {
    return false;
  }
  // Make sure the counts are exhausted.
  std::optional<uint64_t> item;
  if (!counts.tryGetNext(&item, ff.nest(KOSAK_CODING_HERE))) {
    return false;
  }
  if (item.has_value()) {
    return ff.fail(KOSAK_CODING_HERE, "Residual value in counts");
  }
  return true;
}

namespace internal {
template<typename Tuple>
template<size_t Level, typename T>
bool Inflator<Tuple>::tryInflateRecurse(T *result, const FailFrame &ff) {
  // At leaf
  static_assert(Level == std::tuple_size_v<Tuple> - 1);
  *result = std::get<Level>(*current_);
  // Bump current
  return iter_->tryGetNext(&current_, ff.nest(KOSAK_CODING_HERE));
}

template<typename Tuple>
template<size_t Level, typename K, typename V>
bool Inflator<Tuple>::tryInflateRecurse(std::pair<K, V> *result, const FailFrame &ff) {
  result->first = std::get<Level>(*current_);
  return tryInflateRecurse<Level + 1>(&result->second, ff.nest(KOSAK_CODING_HERE));
}

template<typename Tuple>
template<size_t Level, typename ...ARGS>
bool Inflator<Tuple>::tryInflateRecurse(std::tuple<ARGS...> *result, const FailFrame &ff) {
  return tryInflateTupleElementRecurse<Level, 0>(result, ff.nest(KOSAK_CODING_HERE));
}

template<typename Tuple>
template<size_t SrcLevel, size_t DestLevel, typename ...ARGS>
bool Inflator<Tuple>::tryInflateTupleElementRecurse(std::tuple<ARGS...> *result, const FailFrame &ff) {
  if constexpr (DestLevel == sizeof...(ARGS)) {
    // at leaf
    static_assert(SrcLevel == std::tuple_size_v<Tuple>);
    return iter_->tryGetNext(&current_, ff.nest(KOSAK_CODING_HERE));
  } else {
    std::get<DestLevel>(*result) = std::get<SrcLevel>(*current_);
    return tryInflateTupleElementRecurse<SrcLevel + 1, DestLevel + 1>(result, ff.nest(KOSAK_CODING_HERE));
  }
}

template<typename Tuple>
template<size_t Level, typename T>
bool Inflator<Tuple>::tryInflateRecurse(FrozenVector<T> *result, const FailFrame &ff) {
  T *vecData;
  uint64_t size;
  if (!tryGetNextSize(Level, &size, ff.nest(KOSAK_CODING_HERE)) ||
      !alloc_->tryAllocate(size, &vecData, ff.nest(KOSAK_CODING_HERE))) {
    return false;
  }

  for (size_t i = 0; i < size; ++i) {
    if (!tryConfirmHasValue(current_.has_value(), ff.nest(KOSAK_CODING_HERE))) {
      return false;
    }
    auto *item = new((void*)&vecData[i]) T();
    if (!tryInflateRecurse<Level>(item, ff.nest(KOSAK_CODING_HERE))) {
      return false;
    }
  }
  *result = FrozenVector<T>(vecData, size);
  return true;
}

template<typename Tuple>
template<size_t Level, typename T>
bool Inflator<Tuple>::tryInflateRecurse(FrozenSet<T> *result, const FailFrame &ff) {
  FrozenVector<T> vec;
  if (!tryInflateRecurse<Level>(&vec, ff.nest(KOSAK_CODING_HERE))) {
    return false;
  }
  *result = FrozenSet<T>(std::move(vec));
  return true;
}

template<typename Tuple>
template<size_t Level, typename K, typename V>
bool Inflator<Tuple>::tryInflateRecurse(FrozenMap<K, V> *result, const FailFrame &ff) {
  FrozenVector<std::pair<K, V>> vec;
  if (!tryInflateRecurse<Level>(&vec, ff.nest(KOSAK_CODING_HERE))) {
    return false;
  }
  *result = FrozenMap<K, V>(std::move(vec));
  return true;
}
}  // namespace internal
}   // namespace z2kplus::backend::reverse_index::builder::tuple_iterators
