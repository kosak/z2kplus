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

#include <string_view>
#include "kosak/coding/coding.h"
#include "kosak/coding/containers/slice.h"
#include "kosak/coding/dumping.h"
#include "z2kplus/backend/util/frozen/frozen_vector.h"

namespace z2kplus::backend::util::frozen {
template<typename K, typename V>
class FrozenMap {
public:
  typedef K key_type;
  typedef V mapped_type;

  typedef std::pair<K, V> *iterator;
  typedef const std::pair<K, V> *const_iterator;

  FrozenMap() = default;
  explicit FrozenMap(FrozenVector<std::pair<K, V>> &&entries) : entries_(std::move(entries)) {}
  DISALLOW_COPY_AND_ASSIGN(FrozenMap);
  DECLARE_MOVE_COPY_AND_ASSIGN(FrozenMap);
  ~FrozenMap() = default;

  template<typename K2, typename LESS = std::less<K>>
  const_iterator find(const K2 &key, const LESS &less = LESS()) const {
    auto er = equal_range(key, less);
    return er.first == er.second ? end() : er.first;
  }

  template<typename K2, typename LESS = std::less<K>>
  bool tryFind(const K2 &key, const V **result, const LESS &less = LESS()) const {
    auto ip = find(key, less);
    if (ip == end()) {
      return false;
    }
    *result = &ip->second;
    return true;
  }

  template<typename K2, typename LESS = std::less<K>>
  const_iterator lower_bound(const K2 &key, const LESS &less = LESS()) const {
    return equal_range(key, less).first;
  }

  template<typename K2, typename LESS = std::less<K>>
  std::pair<const_iterator, const_iterator> equal_range(const K2 &key, const LESS &less = LESS()) const;

  const_iterator begin() const {
    return entries_.begin();
  }
  const_iterator end() const {
    return entries_.end();
  }

  size_t size() const { return entries_.size(); }

  bool empty() const { return entries_.empty(); }

  FrozenVector<std::pair<K, V>> &entries() { return entries_; }
  const FrozenVector<std::pair<K, V>> &entries() const { return entries_; }

private:
  // We're going to assume, for now, that std::pair is "freezable" for the K and V we care about.
  FrozenVector<std::pair<K, V>> entries_;

  friend std::ostream &operator<<(std::ostream &s, const FrozenMap &o) {
    return s << o.entries_;
  }
};

template<typename K, typename V>
FrozenMap<K, V>::FrozenMap(FrozenMap &&other) noexcept : entries_(std::move(other.entries_)) {
}

template<typename K, typename V>
FrozenMap<K, V> &FrozenMap<K, V>::operator=(FrozenMap &&other) noexcept = default;

template<typename K, typename V>
template<typename K2, typename LESS>
auto FrozenMap<K, V>::equal_range(const K2 &key, const LESS &less) const -> std::pair<const_iterator, const_iterator> {
  struct comparer_t {
    explicit comparer_t(const LESS *less) : less_(less) {}
    bool operator()(const std::pair<K, V> &lhs, const K2 &rhs) const {
      return (*less_)(lhs.first, rhs);
    }
    bool operator()(const K2 &lhs, const std::pair<K, V> &rhs) const {
      return (*less_)(lhs, rhs.first);
    }
    const LESS *less_ = nullptr;
  };
  return std::equal_range(begin(), end(), key, comparer_t(&less));
}
}  // namespace z2kplus::backend::util::frozen
