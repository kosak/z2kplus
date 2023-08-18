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

#include <map>
#include "kosak/coding/coding.h"

namespace kosak::coding {

// For now, this is just a wrapper around map<K, V>. TODO: collapse contiguous items.
template<typename K, typename V>
class RangeMap {
public:
  void insertOrAssign(const K &key, const V &value);
  void insertOrAssign(const K &key, V &&value);

  bool tryFind(const K &key, V **value);
  bool tryFind(const K &key, const V **value) const;

  bool contains(const K &key) const { const V *temp; return tryFind(key, &temp); };
  bool tryRemove(const K &key) { return items_.erase(key) > 0; }

  const std::map<K, V> &items() const { return items_; };

private:
  std::map<K, V> items_;
};

template<typename K, typename V>
void RangeMap<K, V>::insertOrAssign(const K &key, const V &value) {
  items_.insert_or_assign(key, value);
}

template<typename K, typename V>
void RangeMap<K, V>::insertOrAssign(const K &key, V &&value) {
  items_.insert_or_assign(key, std::move(value));
}

template<typename K, typename V>
bool RangeMap<K, V>::tryFind(const K &key, V **value) {
  auto ip = items_.find(key);
  if (ip == items_.end()) {
    return false;
  }
  *value = &ip->second;
  return true;
}

template<typename K, typename V>
bool RangeMap<K, V>::tryFind(const K &key, const V **value) const {
  auto ip = items_.find(key);
  if (ip == items_.end()) {
    return false;
  }
  *value = &ip->second;
  return true;
}

}  // namespace kosak::coding
