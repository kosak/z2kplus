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

#include "kosak/coding/memory/buffered_writer.h"

#define HERE KOSAK_CODING_HERE

namespace kosak::coding::memory {
BufferedWriter::BufferedWriter() = default;
BufferedWriter::BufferedWriter(BufferedWriter &&) noexcept = default;
BufferedWriter &BufferedWriter::operator=(BufferedWriter &&) noexcept = default;
BufferedWriter::BufferedWriter(kosak::coding::nsunix::FileCloser fc) : fc_(std::move(fc)) {}
BufferedWriter::~BufferedWriter() {
  if (!fc_.closed()) {
    warn("Did you fail to call tryClose?");
  }
}

bool BufferedWriter::tryWriteBytes(const char *data, size_t size, const FailFrame &ff) {
  buffer_.append(data, size);
  return tryMaybeFlush(false, ff.nest(HERE));
}

bool BufferedWriter::tryAlign(size_t alignment, const FailFrame &ff) {
  static char zeroes[32];
  passert(alignment <= STATIC_ARRAYSIZE(zeroes));
  auto mask = alignment - 1;
  passert((alignment & mask) == 0, alignment, mask);
  auto residual = offset() & mask;
  if (residual == 0) {
    return true;
  }
  auto paddingSize = alignment - residual;
  return tryWriteBytes(zeroes, paddingSize, ff.nest(HERE));
}

bool BufferedWriter::tryMaybeFlush(bool force, const FailFrame &ff) {
  if (!force && buffer_.size() < highWaterMark) {
    return true;
  }
  if (!nsunix::tryWriteAll(fc_.get(), buffer_.data(), buffer_.size(), ff.nest(HERE))) {
    return false;
  }
  bytesCommitted_ += buffer_.size();
  buffer_.clear();
  return true;
}

bool BufferedWriter::tryClose(const FailFrame &ff) {
  return tryMaybeFlush(true, ff.nest(HERE)) &&
      fc_.tryClose(ff.nest(HERE));
}
}  // namespace kosak::coding::memory
