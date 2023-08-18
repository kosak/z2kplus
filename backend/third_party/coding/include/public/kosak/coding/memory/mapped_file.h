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

#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"

namespace kosak::coding::memory {
namespace internal {

class MappedFileImpl {
  typedef kosak::coding::FailFrame FailFrame;
public:
  MappedFileImpl() = default;
  DISALLOW_COPY_AND_ASSIGN(MappedFileImpl);
  DECLARE_MOVE_COPY_AND_ASSIGN(MappedFileImpl);
  ~MappedFileImpl();

  bool tryMap(const std::string &filename, bool readWrite, const FailFrame &ff);
  bool tryUnmap(const FailFrame &ff);
  bool tryProtect(const FailFrame &ff);

  void *data() { return data_; }
  const void *data() const { return data_; }

  size_t size() const { return size_; }

private:
  void *data_ = nullptr;
  size_t size_ = 0;
};

}  // namespace internal

template<typename T>
class MappedFile {
public:
  bool tryMap(const std::string &filename, bool readWrite, const FailFrame &ff) {
    return impl_.tryMap(filename, readWrite, ff.nest(KOSAK_CODING_HERE));
  }

  bool tryUnmap(const FailFrame &ff) {
    return impl_.tryUnmap(ff.nest(KOSAK_CODING_HERE));
  }

  bool tryProtect(const FailFrame &ff) {
    return impl_.tryProtect(ff.nest(KOSAK_CODING_HERE));
  }

  T *get() { return static_cast<T*>(impl_.data()); }
  const T *get() const { return static_cast<const T*>(impl_.data()); }

  size_t byteSize() const { return impl_.size(); }

  template<typename U>
  void acquire(MappedFile<U> *other) {
    impl_ = std::move(other->impl_);
  }

private:
  internal::MappedFileImpl impl_;

  template<typename U>
  friend class MappedFile;
};
}  // namespace kosak::coding::memory
