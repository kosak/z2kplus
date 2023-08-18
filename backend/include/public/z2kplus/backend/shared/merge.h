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

#if 0
#pragma once
#include <utility>
#include "kosak/coding/coding.h"
#include "z2kplus/backend/util/freezable.h"

namespace z2kplus::backend::shared {

#define SIMPLE_COPY_HELPER(TYPE) \
inline TYPE copyHelper(const TYPE *item) { \
  return *item; \
}
SIMPLE_COPY_HELPER(bool);
SIMPLE_COPY_HELPER(int64);
SIMPLE_COPY_HELPER(uint64);
SIMPLE_COPY_HELPER(AlertState);
SIMPLE_COPY_HELPER(EmotionalReaction);
SIMPLE_COPY_HELPER(ThreadIdClassification);
SIMPLE_COPY_HELPER(kosak::coding::Unit);
SIMPLE_COPY_HELPER(ZgramId);
#undef SIMPLE_COPY_HELPER

inline z2kplus::backend::util::FreezableString copyHelper(
  const z2kplus::backend::util::FreezableString *item) {
  return item->dynamicCopy();
}

inline PerZgramMetadataCore copyHelper(const PerZgramMetadataCore *item) {
  return item->dynamicCopy();
}

inline PerUseridMetadataCore copyHelper(const PerUseridMetadataCore *item) {
  return item->dynamicCopy();
}

template<typename K, typename V>
z2kplus::backend::util::FreezableMap<K, V> copyHelper(
  const z2kplus::backend::util::FreezableMap<K, V> *item) {
  z2kplus::backend::util::FreezableMap<K, V> result;
  std::pair<const K *, const V *> entry;
  for (auto it = item->getMyIterator(); it.tryGetNext(&entry);) {
    result.insertOrAssign(copyHelper(entry.first), copyHelper(entry.second));
  }
  return result;
}

template<typename T1, typename T2>
inline std::pair<T1, T2> copyHelper(const std::pair<T1, T2> *item) {
  return std::make_pair(copyHelper(&item->first), copyHelper(&item->second));
};

#define SIMPLE_MERGE_FROM(TYPE) \
inline void mergeFrom(TYPE *dest, const TYPE &src, bool /*collapseFalseish*/) { \
  *dest = src; \
} \
inline void destructivelyMergeFrom(TYPE *dest, TYPE &&src, bool /*collapseFalseish*/) { \
  *dest = src; \
}
SIMPLE_MERGE_FROM(bool);
SIMPLE_MERGE_FROM(int64);
SIMPLE_MERGE_FROM(uint64);
SIMPLE_MERGE_FROM(AlertState);
SIMPLE_MERGE_FROM(EmotionalReaction);
SIMPLE_MERGE_FROM(ThreadIdClassification);
SIMPLE_MERGE_FROM(kosak::coding::Unit);
SIMPLE_MERGE_FROM(ZgramId);
#undef SIMPLE_MERGE_FROM

inline void mergeFrom(z2kplus::backend::util::FreezableString *dest,
  const z2kplus::backend::util::FreezableString &src, bool /*collapseFalseish*/) {
  *dest = src.dynamicCopy();
}
inline void destructivelyMergeFrom(z2kplus::backend::util::FreezableString *dest,
  z2kplus::backend::util::FreezableString &&src, bool /*collapseFalseish*/) {
  *dest = std::move(src);
}

template<typename T1, typename T2>
inline void mergeFrom(std::pair<T1, T2> *dest, const std::pair<T1, T2> &src, bool collapseFalsish) {
  mergeFrom(&dest->first, src.first, collapseFalsish);
  mergeFrom(&dest->second, src.second, collapseFalsish);
};

template<typename T1, typename T2>
inline void destructivelyMergeFrom(std::pair<T1, T2> *dest, std::pair<T1, T2> &&src,
  bool collapseFalsish) {
  destructivelyMergeFrom(&dest->first, std::move(src.first), collapseFalsish);
  destructivelyMergeFrom(&dest->second, std::move(src.second), collapseFalsish);
};

inline void destructivelyMergeFrom(PerZgramMetadataCore *dest, PerZgramMetadataCore &&src,
  bool collapseFalseish) {
  dest->destructivelyMergeFrom(std::move(src), collapseFalseish);
}

inline void destructivelyMergeFrom(PerUseridMetadataCore *dest, PerUseridMetadataCore &&src,
  bool collapseFalseish) {
  dest->destructivelyMergeFrom(std::move(src), collapseFalseish);
}

template<typename K, typename V>
void mergeFrom(z2kplus::backend::util::FreezableMap<K, V> *dest,
  const z2kplus::backend::util::FreezableMap<K, V> &src, bool collapseFalseish) {
  std::pair<const K*, const V*> srcEntry;
  std::pair<const K*, V*> destEntry;
  for (auto it = src.getMyIterator(); it.tryGetNext(&srcEntry) ; ) {
    const auto &key = *srcEntry.first;
    const auto &innerSrc = *srcEntry.second;
    if (!dest->tryFind(key, &destEntry)) {
      dest->insertOrAssign(copyHelper(&key), copyHelper(&innerSrc), &destEntry);
    } else {
      mergeFrom(destEntry.second, innerSrc, collapseFalseish);
    }
    if (collapseFalseish && isFalseish(destEntry.second)) {
      dest->tryRemove(key);
    }
  }
};

template<typename K, typename V>
void destructivelyMergeFrom(z2kplus::backend::util::FreezableMap<K, V> *dest,
  z2kplus::backend::util::FreezableMap<K, V> &&src, bool collapseFalseish) {
  if (dest->empty()) {
    *dest = std::move(src);
    if (!collapseFalseish) {
      return;
    }
    std::vector<const K *> toRemove;
    std::pair<const K *, V *> srcEntry;
    for (auto it = src.getMyIterator(); it.tryGetNext(&srcEntry);) {
      if (isFalseish(srcEntry.second)) {
        toRemove.push_back(srcEntry.first);
      }
    }
    for (const auto *key : toRemove) {
      src.tryRemove(*key);
    }
    return;
  }
  std::pair<const K*, V*> srcEntry;
  std::pair<const K*, V*> destEntry;
  for (auto it = src.getMyIterator(); it.tryGetNext(&srcEntry) ; ) {
    const auto &key = *srcEntry.first;
    auto &innerSrc = *srcEntry.second;
    if (!dest->tryFind(key, &destEntry)) {
      dest->insertOrAssign(copyHelper(&key), std::move(innerSrc), &destEntry);
    } else {
      destructivelyMergeFrom(destEntry.second, std::move(innerSrc), collapseFalseish);
    }
    if (collapseFalseish && isFalseish(destEntry.second)) {
      dest->tryRemove(key);
    }
  }
}

}  // namespace z2kplus::backend::shared
#endif
