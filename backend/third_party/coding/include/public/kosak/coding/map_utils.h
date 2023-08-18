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
#include "kosak/coding/failures.h"

namespace kosak::coding {
namespace maputils {

template<typename K1, typename K2, typename V, typename C, typename A>
bool tryFind(std::map<K1, V, C, A> &map, const K2 &key, V **result) {
  auto ip = map.find(key);
  if (ip == map.end()) {
    return false;
  }
  *result = &ip->second;
  return true;
}

template<typename K1, typename K2, typename V, typename C, typename A>
bool tryFind(std::map<K1, V, C, A> &map, const K2 &key, V **result,
    const kosak::coding::FailFrame &ff) {
  if (!tryFind(map, key, result)) {
    ff.failf(KOSAK_CODING_HERE, "tryFind(%o) failed", key);
    return false;
  }
  return true;
}

template<typename K1, typename K2, typename V, typename C, typename A>
bool tryFind(const std::map<K1, V, C, A> &map, const K2 &key, const V **result) {
  auto ip = map.find(key);
  if (ip == map.end()) {
    return false;
  }
  *result = &ip->second;
  return true;
}

template<typename K1, typename K2, typename V, typename C, typename A>
bool tryFind(const std::map<K1, V, C, A> &map, const K2 &key, V **result,
    const kosak::coding::FailFrame &ff) {
  if (!tryFind(map, key, result)) {
    ff.failf(KOSAK_CODING_HERE, "tryFind(%o) failed", key);
    return false;
  }
  return true;
}

template<typename K1, typename K2, typename V, typename C, typename A>
V &findOrDefault(std::map<K1, V, C, A> &map, const K2 &key, V &deflt) {
  auto ip = map.find(key);
  if (ip == map.end()) {
    return deflt;
  }
  return ip->second;
}

template<typename K1, typename K2, typename V, typename C, typename A>
const V &findOrDefault(const std::map<K1, V, C, A> &map, const K2 &key, const V &deflt) {
  auto ip = map.find(key);
  if (ip == map.end()) {
    return deflt;
  }
  return ip->second;
}

template<typename K1, typename K2, typename V, typename C, typename A>
V &mustGet(std::map<K1, V, C, A> &map, const K2 &key) {
  V *result;
  passert(tryFind(map, key, &result));
  return *result;
}

//template<typename K, typename V, typename C, typename A>
//bool tryFind(std::map<K, V, C, A> &m, const K &key,
//    typename std::map<K, V, C, A>::iterator *result, Failures *optionalFailures) {
//  *result = m.find(key);
//  if (*result != m.end()) {
//    return true;
//  }
//  if (optionalFailures != nullptr) {
//    optionalFailures->add("Can't find %o in map", key);
//  }
//  return false;
//}
//
//template<typename K, typename V, typename C, typename A>
//bool tryFind(const std::map<K, V, C, A> &m, const K &key,
//    typename std::map<K, V, C, A>::const_iterator *result, Failures *optionalFailures) {
//  *result = m.find(key);
//  if (*result != m.end()) {
//    return true;
//  }
//  if (optionalFailures != nullptr) {
//    optionalFailures->add("Can't find %o in map", key);
//  }
//  return false;
//}
//
//
//template<typename K, typename V, typename C, typename A>
//bool tryFind(std::map<K, V, C, A> &m, const K &key, V **result, Failures *optionalFailures) {
//  typename std::map<K, V, C, A>::iterator ip;
//  if (!tryFind(m, key, &ip, optionalFailures)) {
//    return false;
//  }
//  *result = &ip->second;
//  return true;
//}
//
//template<typename K, typename V, typename C, typename A>
//bool tryFind(const std::map<K, V, C, A> &m, const K &key, const V **result, Failures *optionalFailures) {
//  typename std::map<K, V, C, A>::const_iterator ip;
//  if (!tryFind(m, key, &ip, optionalFailures)) {
//    return false;
//  }
//  *result = &ip->second;
//  return true;
//}
//
//
// template<typename K, typename V, typename C, typename A>
// V *mustFind(const std::map<K, V, C, A> &m, const K &key)
//  Failures failures;
//  V *result;
//  if (!tryFind(m, key, &result, &failures)) {
//    crash("mustFind failed: %o", failures);
//  }
//  return result;
//}

//
//template<typename K, typename V, typename C, typename A>
//const V *mustFind(const std::map<K, V, C, A> &m, const K &key) {
//  Failures failures;
//  const V *result;
//  if (!tryFind(m, key, &result, &failures)) {
//    crash("mustFind failed: %o", failures);
//  }
//  return result;
//}

template<typename K1, typename K2, typename V, typename C, typename A>
bool tryExtract(std::map<K1, V, C, A> *m, const K2 &key, typename std::map<K1, V, C, A>::node_type *result,
    const kosak::coding::FailFrame &ff) {
  *result = m->extract(key);
  if (!result->empty()) {
    return true;
  }
  ff.failf(KOSAK_CODING_HERE, "Can't find %o in map", key);
  return false;
}
}  // namespace maputils
}  // namespace kosak::coding
