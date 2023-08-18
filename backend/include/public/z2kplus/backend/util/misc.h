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

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "kosak/coding/coding.h"
#include "z2kplus/backend/util/myallocator.h"

namespace z2kplus::backend::util {

struct Identity {
  template<typename T>
  const T &operator()(const T &item) const { return item; }
};

struct UniversalLess {
  template<typename T>
  bool operator()(const T &lhs, const T &rhs) const { return lhs < rhs; }
};

template<typename InnerComparer = UniversalLess>
struct FirstLess {
  FirstLess(InnerComparer inner = InnerComparer()) : inner_(inner) {}

  template<typename T1, typename T2>
  bool operator()(const std::pair<T1, T2> &lhs, const std::pair<T1, T2> &rhs) const {
    return inner_(lhs.first, rhs.first);
  }

  InnerComparer inner_;
};

template<typename InnerComparer = UniversalLess>
struct DerefLess {
  DerefLess(InnerComparer inner = InnerComparer()) : inner_(inner) {}

  template<typename T>
  bool operator()(const T *lhs, const T *rhs) const {
    return inner_(*lhs, *rhs);
  }

  InnerComparer inner_;
};

template<typename InnerComparer = UniversalLess>
struct ParityLess {
  ParityLess(bool forward = true, InnerComparer inner = InnerComparer()) : forward_(forward),
    inner_(inner) {}

  template<typename T>
  bool operator()(const T &lhs, const T &rhs) const {
    return forward_ ? inner_(lhs, rhs) : inner_(rhs, lhs);
  }

  bool forward_;
  InnerComparer inner_;
};

inline int compareUint32(uint32 lhs, uint32 rhs) {
  if (lhs < rhs) {
    return -1;
  }
  if (lhs > rhs) {
    return 1;
  }
  return 0;
}

inline int compareInt64(int64 lhs, int64 rhs) {
  if (lhs < rhs) {
    return -1;
  }
  if (lhs > rhs) {
    return 1;
  }
  return 0;
}

inline int compareUint64(uint64 lhs, uint64 rhs) {
  if (lhs < rhs) {
    return -1;
  }
  if (lhs > rhs) {
    return 1;
  }
  return 0;
}

inline int compareBool(bool lhs, bool rhs) {
  return compareUint32((uint32)lhs, (uint32)rhs);
}

class Uint32Comparer {
public:
  static int compare(uint32 lhs, uint32 rhs) {
    return compareUint32(lhs, rhs);
  }
};

template<typename Iterator, typename UintType>
bool tryRangeToUint(Iterator begin, Iterator end, UintType *result) {
  if (begin == end) {
    return false;
  }
  *result = 0;
  for (; begin != end; ++begin) {
    char ch = *begin;
    if (!isdigit(ch)) {
      return false;
    }
    *result = *result * 10 + ch - '0';
  }
  return true;
}

template<typename K, typename V, typename C, typename A>
bool tryFind(std::map<K, V, C, A> &map, const K &key, V **result) {
  auto ip = map.find(key);
  if (ip == map.end()) {
    return false;
  }
  *result = &ip->second;
  return true;
}

template<typename K, typename V, typename C, typename A>
bool tryFind(const std::map<K, V, C, A> &map, const K &key, const V **result) {
  auto ip = map.find(key);
  if (ip == map.end()) {
    return false;
  }
  *result = &ip->second;
  return true;
}

template<typename K, typename V, typename C, typename A>
V &findOrDefault(std::map<K, V, C, A> &map, const K &key, V &deflt) {
  auto ip = map.find(key);
  if (ip == map.end()) {
    return deflt;
  }
  return ip->second;
}

template<typename K, typename V, typename C, typename A>
const V &findOrDefault(const std::map<K, V, C, A> &map, const K &key, const V &deflt) {
  auto ip = map.find(key);
  if (ip == map.end()) {
    return deflt;
  }
  return ip->second;
}

template<typename K, typename V, typename C, typename A>
V &mustGet(std::map<K, V, C, A> &map, const K &key) {
  V *result;
  passert(tryFind(map, key, &result));
  return *result;
}

template<typename T>
class HalfOpenInterval {
public:
  HalfOpenInterval(T begin, T end) : begin_(std::move(begin)), end_(std::move(end)) {}

  bool intersects(const HalfOpenInterval &other) const;
  HalfOpenInterval expand(const HalfOpenInterval &other) const;

  bool empty() const { return begin_ == end_; }

private:
  T begin_;
  T end_;
};

template<typename T>
bool HalfOpenInterval<T>::intersects(const HalfOpenInterval &other) const {
  if (!(begin_ < other.end_)) {
    return false;
  }
  return other.begin_ < end_;
}

template<typename T>
HalfOpenInterval<T> HalfOpenInterval<T>::expand(const HalfOpenInterval &other) const {
  if (empty()) {
    return other;
  }
  if (other.empty()) {
    return *this;
  }
  const auto &resultBegin = begin_ < other.begin_ ? begin_ : other.begin_;  // min
  const auto &resultEnd = end_ < other.end_ ? other.end_ : end_;  // max
  return HalfOpenInterval(resultBegin, resultEnd);
}

}  // namespace z2kplus::backend::util
#endif
