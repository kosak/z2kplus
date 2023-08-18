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

#include <iostream>
#include <vector>

namespace kosak::coding::containers {
template<typename T>
class Slice {
public:
  Slice() : begin_(nullptr), end_(nullptr) {}
  Slice(T *begin, T *end) : begin_(begin), end_(end) {}
  Slice(T *begin, size_t size) : begin_(begin), end_(begin + size) {}

  T &front() const { return *begin_; }
  T &back() const { return *(end_ - 1); }
  T &operator[](size_t index) const { return begin_[index]; }

  T *data() const { return begin_; }
  T *begin() const { return begin_; }
  T *end() const { return end_; }

  size_t size() const { return end_ - begin_; }
  bool empty() const { return size() == 0; }

  int compare(const Slice &other) const;

private:
  T *begin_ = nullptr;
  T *end_ = nullptr;
};

template<typename T>
bool operator==(const Slice<T> &lhs, const Slice<T> &rhs) {
  return lhs.size() == rhs.size() && lhs.compare(rhs) == 0;
}

template<typename T>
int Slice<T>::compare(const Slice &other) const {
  size_t minSize = std::min(size(), other.size());
  for (size_t i = 0; i < minSize; ++i) {
    const auto &lItem = (*this)[i];
    const auto &rItem = other[i];
    if (lItem < rItem) {
      return -1;
    }
    if (lItem > rItem) {
      return 1;
    }
  }
  if (size() > minSize) {
    return 1;
  }
  if (other.size() > minSize) {
    return -1;
  }
  return 0;
}

template<typename T>
std::ostream &operator<<(std::ostream &s, const Slice<T> &o) {
  s << '[';
  const char *comma = "";
  for (const auto &item : o) {
    s << comma << item;
    comma = ", ";
  }
  return s << ']';
}

template<typename T>
Slice<const T> asSlice(const std::vector<T> &vec) {
  return {vec.data(), vec.data() + vec.size()};
}
}  // namespace kosak::coding::containers
