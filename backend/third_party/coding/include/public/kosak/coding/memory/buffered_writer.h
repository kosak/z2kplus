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
#include "kosak/coding/unix.h"

namespace kosak::coding::memory {
class BufferedWriter {
  typedef kosak::coding::FailFrame FailFrame;
  static constexpr size_t highWaterMark = 16384;

public:
  BufferedWriter();
  explicit BufferedWriter(kosak::coding::nsunix::FileCloser fc);
  DISALLOW_COPY_AND_ASSIGN(BufferedWriter);
  DECLARE_MOVE_COPY_AND_ASSIGN(BufferedWriter);
  ~BufferedWriter();

  size_t offset() const { return bytesCommitted_ + buffer_.size(); }

  bool tryWriteBytes(const char *data, size_t size, const FailFrame &ff);

  bool tryWriteByte(uint8_t item, const FailFrame &ff) {
    const char *data = bit_cast<const char*>(&item);
    return tryWriteBytes(data, sizeof(item), ff);
  }

  bool tryWriteUint32(uint32_t item, const FailFrame &ff) {
    const char *data = bit_cast<const char*>(&item);
    return tryWriteBytes(data, sizeof(item), ff);
  }

  bool tryWriteInt64(int64_t item, const FailFrame &ff) {
    const char *data = bit_cast<const char*>(&item);
    return tryWriteBytes(data, sizeof(item), ff);
  }

  bool tryWriteUint64(uint64_t item, const FailFrame &ff) {
    const char *data = bit_cast<const char*>(&item);
    return tryWriteBytes(data, sizeof(item), ff);
  }

  bool tryWriteChar32(char32_t item, const FailFrame &ff) {
    const char *data = bit_cast<const char*>(&item);
    return tryWriteBytes(data, sizeof(item), ff);
  }

  template<typename T>
  bool tryWritePOD(const T *data, size_t size, const FailFrame &ff) {
    const auto *start = bit_cast<const char*>(data);
    return tryWriteBytes(start, size * sizeof(T), ff);
  }

  bool tryAlign(size_t alignment, const FailFrame &ff);

  bool tryMaybeFlush(bool force, const FailFrame &ff);
  bool tryClose(const FailFrame &ff);

  std::string *getBuffer() { return &buffer_; }

private:
  kosak::coding::nsunix::FileCloser fc_;
  std::string buffer_;
  size_t bytesCommitted_ = 0;
};
}  // namespace zakosak::coding::memory
