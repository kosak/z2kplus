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

#include <mutex>
#include <condition_variable>
#include "kosak/coding/coding.h"
#include "kosak/coding/misc.h"

namespace kosak::coding {
namespace internal {
class RefAccounting {
public:
  RefAccounting();
  DISALLOW_COPY_AND_ASSIGN(RefAccounting);
  DISALLOW_MOVE_COPY_AND_ASSIGN(RefAccounting);
  ~RefAccounting();

  void ref();
  void unref();
  bool isUnique();

private:
  std::mutex mutex_;
  std::condition_variable condVar_;
  size_t references_ = 0;
};
}  // namespace internal

template<typename T>
class Ref;

template<typename T>
class RefCounted {
public:
  template<typename ...ARGS>
  explicit RefCounted(ARGS &&...args);
  DISALLOW_COPY_AND_ASSIGN(RefCounted);
  DISALLOW_MOVE_COPY_AND_ASSIGN(RefCounted);
  ~RefCounted();

  Ref<T> makeRef();

  bool isUnique() {
    return accounting_.isUnique();
  }

  T *get() { return &item_; }
  const T *get() const { return &item_; }

  T &operator*() { return item_; }
  const T &operator*() const { item_; }

  T *operator->() { return &item_; }
  const T *operator->() const { return &item_; }

private:
  // It is desirable for item to be first, and accounting to be last, so that:
  // 1. The getter methods for item_ don't have to add an offset
  // 2. The destructor for accounting_ (which waits for the refcount to reach zero) comes after the
  //    destructor for item, so we can use the default.
  T item_;
  internal::RefAccounting accounting_;

  friend class Ref<T>;
};

template<typename T>
class Ref {
public:
  Ref() = default;
  DECLARE_COPY_AND_ASSIGN(Ref);
  DEFINE_MOVE_COPY_AND_ASSIGN(Ref);
  ~Ref() {
    reset();
  }

  void reset() {
    if (owner_ == nullptr) {
      return;
    }
    auto *ownerp = owner_.get();
    owner_.reset();
    ownerp->accounting_.unref();
  }

  T *get() { return owner_->get(); }
  const T *get() const { owner_->get(); }

  T &operator*() { return owner_->operator*(); }
  const T &operator*() const { return owner_->operator*(); }

  T *operator->() { return owner_->operator->(); }
  const T *operator->() const { return owner_->operator->(); }

private:
  explicit Ref(RefCounted<T> *owner) : owner_(owner) {}
  kosak::coding::UnownedPtr<RefCounted<T>> owner_;

  friend class RefCounted<T>;
};

template<typename T>
template<typename ...ARGS>
RefCounted<T>::RefCounted(ARGS &&...args) : item_(std::forward<ARGS>(args)...) {}

template<typename T>
RefCounted<T>::~RefCounted() = default;

template<typename T>
Ref<T> RefCounted<T>::makeRef() {
  accounting_.ref();
  return Ref<T>(this);
}

template<typename T>
Ref<T>::Ref(const Ref &other) : owner_(other.owner_.get()) {
  if (owner_ != nullptr) {
    owner_->accounting_.ref();
  }
}

template<typename T>
Ref<T> &Ref<T>::operator=(const Ref &other) {
  Ref temp(other);
  owner_.swap(temp.owner_);
  return *this;
}
}  // namespace kosak::coding
