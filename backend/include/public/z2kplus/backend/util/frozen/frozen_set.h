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
#include "z2kplus/backend/util/frozen/frozen_vector.h"

namespace z2kplus::backend::util::frozen {

template<typename T>
class FrozenSet {
public:
  FrozenSet() = default;
  explicit FrozenSet(FrozenVector<T> &&entries) : entries_(std::move(entries)) {}
  DISALLOW_COPY_AND_ASSIGN(FrozenSet);
  DEFINE_MOVE_COPY_AND_ASSIGN(FrozenSet);
  ~FrozenSet() = default;

  typedef T key_type;
  typedef T value_type;

  typedef const T *iterator;
  typedef const T *const_iterator;

  const T *begin() const { return entries_.begin(); }
  const T *end() const { return entries_.end(); }

  FrozenVector<T> &entries() { return entries_; }
  const FrozenVector<T> &entries() const { return entries_; }

  size_t size() const { return entries_.size(); }

  template<typename T2, typename LESS = std::less<T>>
  const_iterator find(const T2 &key, const LESS &less = LESS()) const {
    auto er = equal_range(key, less);
    return er.first == er.second ? end() : er.first;
  }

  template<typename T2, typename LESS = std::less<T>>
  bool tryFind(const T2 &key, const T **result, const LESS &less = LESS()) const {
    auto ip = find(key, less);
    if (ip == end()) {
      return false;
    }
    *result = &*ip;
    return true;
  }

  template<typename T2, typename LESS = std::less<T>>
  const_iterator lower_bound(const T2 &key, const LESS &less = LESS()) const {
    return equal_range(key, less).first;
  }

  template<typename T2, typename LESS = std::less<T>>
  std::pair<const_iterator, const_iterator> equal_range(const T2 &key, const LESS &less = LESS()) const {
    return std::equal_range(begin(), end(), key, less);
  }

private:
  FrozenVector<T> entries_;

  friend std::ostream &operator<<(std::ostream &s, const FrozenSet &o) {
    return s << o.entries_;
  }
};
}  // namespace z2kplus::backend::util::frozen
