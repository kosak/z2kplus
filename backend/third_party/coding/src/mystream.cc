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

#include "kosak/coding/mystream.h"

#include <cstring>

namespace kosak::coding {

MyOstringStream::MyOstringStream() : std::ostream(this), dest_(&internalBuffer_) {}
MyOstringStream::MyOstringStream(std::string *clientBuffer) : std::ostream(this),
  dest_(clientBuffer) {}
MyOstringStream::~MyOstringStream() = default;

MyOstringStream::Buf::int_type MyOstringStream::overflow(int c) {
  if (!Buf::traits_type::eq_int_type(c, Buf::traits_type::eof())) {
    dest_->push_back(static_cast<char>(c));
  }
  return c;
}

std::streamsize MyOstringStream::xsputn(const char* s, std::streamsize n) {
  dest_->append(s, n);
  return n;
}

BufferStream::BufferStream(char *data, size_t capacity) : std::ostream(this), current_(data),
    end_(data + capacity) {}

BufferStream::~BufferStream() = default;

BufferStream::Buf::int_type BufferStream::overflow(int c) {
  if (current_ == end_) {
    return Buf::traits_type::eof();
  }
  if (!Buf::traits_type::eq_int_type(c, Buf::traits_type::eof())) {
    *current_++ = static_cast<char>(c);
  }
  return c;
}

std::streamsize BufferStream::xsputn(const char *s, std::streamsize n) {
  auto space = end_ - current_;
  auto bytesToWrite = std::min(n, space);
  std::memcpy(current_, s, bytesToWrite);
  current_ += bytesToWrite;
  return bytesToWrite;
}

LimitStream::LimitStream(std::ostream *inner, size_t limit) : std::ostream(this), inner_(inner),
  count_(limit) {}

LimitStream::Buf::int_type LimitStream::overflow(int c) {
  if (!Buf::traits_type::eq_int_type(c, Buf::traits_type::eof()) && count_ > 0) {
    --count_;
    (*inner_) << (char)c;
  }
  return c;
}

std::streamsize LimitStream::xsputn(const char *s, std::streamsize n) {
  if (count_ == 0) {
    return 0;
  }
  size_t amountToSend = std::min<size_t>(count_, n);
  count_ -= amountToSend;
  inner_->write(s, amountToSend);
  return amountToSend;
}

}  // namespace kosak::coding
