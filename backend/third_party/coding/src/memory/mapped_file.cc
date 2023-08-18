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

#include "kosak/coding/memory/mapped_file.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <string>
#include <string_view>

#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/unix.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::nsunix::FileCloser;
namespace nsunix = kosak::coding::nsunix;

#define HERE KOSAK_CODING_HERE

namespace kosak::coding::memory {

namespace internal {
MappedFileImpl::MappedFileImpl(MappedFileImpl &&other) noexcept : data_(other.data_),
  size_(other.size_) {
  other.data_ = nullptr;
  other.size_ = 0;
}

MappedFileImpl &MappedFileImpl::operator=(MappedFileImpl &&other) noexcept {
  FailRoot failRoot;
  if (!tryUnmap(failRoot.nest(HERE))) {
    crash("%o", failRoot);
  }
  data_ = other.data_;
  size_ = other.size_;
  other.data_ = nullptr;
  other.size_ = 0;
  return *this;
}

MappedFileImpl::~MappedFileImpl() {
  FailRoot failRoot;
  if (!tryUnmap(failRoot.nest(HERE))) {
    crash("%o", failRoot);
  }
}

bool MappedFileImpl::tryMap(const std::string &filename, bool readWrite, const FailFrame &ff) {
  FileCloser fc;
  if (!tryUnmap(ff.nest(HERE)) ||
    !nsunix::tryOpen(filename, readWrite ? O_RDWR : O_RDONLY, 0, &fc, ff.nest(HERE))) {
    return false;
  }
  struct stat stat = {};
  if (!nsunix::tryFstat(fc.get(), &stat, ff.nest(HERE))) {
    return false;
  }

  auto size = static_cast<size_t>(stat.st_size);
  if (size == 0) {
    data_ = nullptr;
    size_ = 0;
    return true;
  }

  auto prot = readWrite ? PROT_READ | PROT_WRITE : PROT_READ;
  auto p = mmap(nullptr, size, prot, MAP_SHARED, fc.get(), 0);
  if (p == MAP_FAILED) {
    return ff.failf(HERE, "Can't mmap file %o, errno=%o", filename, errno);
  }

  data_ = p;
  size_ = size;
  return true;
}

bool MappedFileImpl::tryUnmap(const FailFrame &ff) {
  if (data_ == nullptr) {
    return true;
  }

  if (munmap(data_, size_) < 0) {
    return ff.failf(HERE, "munmap failed failed, errno=%o", errno);
  }
  data_ = nullptr;
  size_ = 0;
  return true;
}

bool MappedFileImpl::tryProtect(const FailFrame &ff) {
  if (mprotect(data_, size_, PROT_READ) < 0) {
    return ff.failf(HERE, "mprotect failed, errno=%o", errno);
  }
  return true;
}
}  // namespace internal
}  // namespace kosak::coding::memory
