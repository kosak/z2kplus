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
#include "kosak/coding/dumping.h"
#include "z2kplus/backend/util/relative.h"

namespace z2kplus::backend::util::frozen {
template<typename T>
class FrozenVector {
public:
  FrozenVector(T *data, size_t size) : data_(data), size_(size) {}
  FrozenVector() : data_(nullptr), size_(0) {}
  DISALLOW_COPY_AND_ASSIGN(FrozenVector);
  DECLARE_MOVE_COPY_AND_ASSIGN(FrozenVector);
  ~FrozenVector() = default;

public:
  void push_back(T data) {
    data_.get()[size_++] = std::move(data);
  }
  const T &operator[](size_t index) const { return data_.get()[index]; }
  T &back() { return *begin(); }
  const T &back() const { return *begin(); }

  size_t size() const { return size_; }
  bool empty() const { return size() == 0; }

  T *begin() { return data_.get(); }
  const T *begin() const { return data_.get(); }

  T *end() { return begin() + size_; }
  const T *end() const { return begin() + size_; }

  const T *data() const { return data_.get(); }

private:
  RelativePtr<T> data_;
  size_t size_ = 0;
};

template<typename T>
FrozenVector<T>::FrozenVector(FrozenVector &&other) noexcept : data_(other.data_),
  size_(other.size_) {
  other.data_.set(nullptr);
  other.size_ = 0;
}

template<typename T>
FrozenVector<T> &FrozenVector<T>::operator=(FrozenVector &&other) noexcept {
  data_ = other.data_;
  size_ = other.size_;
  other.data_.set(nullptr);
  other.size_ = 0;
  return *this;
}

template<typename T>
std::ostream &operator<<(std::ostream &s, const FrozenVector<T> &o) {
  return s << kosak::coding::dump(o.begin(), o.end(), "[", "]", ",");
}
}  // namespace z2kplus::backend::util::frozen
