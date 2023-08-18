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

namespace z2kplus::backend::util {

using kosak::coding::streamf;

namespace relativePtr {
inline ssize_t calcRaw(size_t self, size_t target) {
  if (self < target) {
    return (ssize_t)(target - self);
  }
  return -(ssize_t)(self - target);
}
}  // namespace relativePtr

template<typename T>
class RelativePtr {
public:
  RelativePtr() = default;
  explicit RelativePtr(T *p) { set(p); }
  RelativePtr(const RelativePtr &other) { set(other.get()); }
  RelativePtr &operator=(const RelativePtr &other) { set(other.get()); return *this; }
  RelativePtr(RelativePtr &&other) noexcept {
    set(other.get());
    other.set(nullptr);
  }
  RelativePtr &operator=(RelativePtr &&other) noexcept {
    set(other.get());
    other.set(nullptr);
    return *this;
  }
  ~RelativePtr() = default;

  void set(T *p);
  T *get() const;

  explicit operator T*() const { return get(); }

  T &operator[](ssize_t index) const { return get()[index]; }

  int64_t raw() const { return offset_; }

private:
  // We need a representation for nullptr. We might think we should use 0, but a relative value of 0
  // is problematic because it would prevent valid use cases like RelativePtr<Node>->get()->nextp
  // pointing to itself. So we use a value of 1. This points into a location in the interior of this
  // RelativePtr and so would never be a valid use case.
  static constexpr int64_t nullOffset = 1;
  int64_t offset_ = nullOffset;
};

template<typename T>
void RelativePtr<T>::set(T *p) {
  if (p == nullptr) {
    offset_ = nullOffset;
    return;
  }
  auto offset = reinterpret_cast<intptr_t>(p) - reinterpret_cast<intptr_t>(this);
  passert(offset != nullOffset);
  offset_ = static_cast<int64>(offset);
}

template<typename T>
T *RelativePtr<T>::get() const {
  if (offset_ == nullOffset) {
    return nullptr;
  }
  return reinterpret_cast<T*>(reinterpret_cast<intptr_t>(this) + offset_);
}

template<typename T>
RelativePtr<T> toRelativePtr(T *p) {
  return RelativePtr<T>(p);
}

//template<typename T>
//class RelativeSlice {
//public:
//  RelativeSlice() = default;
//  RelativeSlice(T *data, size_t size) : data_(data), size_(size) {}
//  RelativeSlice(const RelativeSlice &other) = default;
//  RelativeSlice &operator=(const RelativeSlice &other) = default;
//  RelativeSlice(RelativeSlice &&other) noexcept = default;
//  RelativeSlice &operator=(RelativeSlice &&other) noexcept = default;
//  RelativeSlice(const Slice<T> &other) : data_(other.data()), size_(other.size()) {}  // NOLINT
//  ~RelativeSlice() = default;
//
//  RelativeSlice<const T> toConst() const {
//    return RelativeSlice<const T>(data_.get(), size_);
//  }
//
//  Slice<T> toSlice() const {
//    return Slice<T>(data_.get(), size_);
//  }
//
//  T *data() const { return data_.get(); }
//  T *begin() const { return data(); }
//  T *end() const { return data() + size_; }
//  T &operator[](size_t index) const { return *(data() + index); }
//
//  size_t size() const { return size_; }
//  bool empty() const { return size_ == 0; }
//
//  T &front() const { return (*this)[0]; }
//  T &back() const { return (*this)[size_ - 1]; }
//
//  typedef T value_type;
//
//protected:
//  RelativePtr<T> data_;
//  size_t size_ = 0;
//};
//
//template<typename T>
//RelativeSlice<T> toRelativeSlice(const Slice <T> &slice) {
//  return RelativeSlice<T>(slice);
//}
//
//template<typename T>
//std::ostream &operator<<(std::ostream &s, const RelativeSlice<T> &vec) {
//  s << '[';
//  const char *comma = "";
//  for (const auto &element : vec) {
//    s << comma;
//    s << element;
//    comma=", ";
//  }
//  s << ']';
//  return s;
//}

}  // namespace z2kplus::backend::util
