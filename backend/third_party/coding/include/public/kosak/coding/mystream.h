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
#include <ostream>
#include <streambuf>
#include <string>

namespace kosak::coding {

// A more efficient ostringstream that allows you to grab the internal buffer if you want it.
// Or, if you don't want to use the internal buffer, it allows you to provide your own.
class MyOstringStream final : private std::basic_streambuf<char>, public std::ostream {
  using Buf = std::basic_streambuf<char>;
public:
  MyOstringStream();
  explicit MyOstringStream(std::string *clientBuffer);
  MyOstringStream(const MyOstringStream &other) = delete;
  MyOstringStream &operator=(const MyOstringStream &other) = delete;
  ~MyOstringStream() final;

  const std::string &str() const { return *dest_; }
  std::string &str() { return *dest_; }

private:
  Buf::int_type overflow(int c) final;
  std::streamsize xsputn(const char *s, std::streamsize n) final;

  std::string internalBuffer_;
  std::string *dest_;
};

class BufferStream final : private std::basic_streambuf<char>, public std::ostream {
  using Buf = std::basic_streambuf<char>;
public:
  BufferStream(char *data, size_t capacity);
  ~BufferStream() final;

  BufferStream(const BufferStream &other) = delete;
  BufferStream &operator=(const BufferStream &other) = delete;

  char *current() const { return current_; }

private:
  Buf::int_type overflow(int c) final;
  std::streamsize xsputn(const char *s, std::streamsize n) final;

  char *current_ = nullptr;
  const char *end_ = nullptr;
};


// A stream that forwards to another stream, but that stops forwarding after a specified number of
// characters has been transferred.
class LimitStream final : private std::basic_streambuf<char>, public std::ostream {
  using Buf = std::basic_streambuf<char>;
public:
  LimitStream(std::ostream *inner, size_t limit);
  LimitStream(const LimitStream &other) = delete;
  LimitStream &operator=(const LimitStream &other) = delete;

private:
  Buf::int_type overflow(int c) final;
  std::streamsize xsputn(const char *s, std::streamsize n) final;

  // Does not own.
  std::ostream * const inner_ = nullptr;
  size_t count_ = 0;
};

// An ostream adapter that uses LimitStream
template<typename T>
class Limit {
public:
  Limit(const T &item, size_t size) : item_(item), size_(size) {}
private:
  const T &item_;
  const size_t size_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const Limit &o) {
    LimitStream ls(&s, o.size_);
    ls << o.item_;
    return s;
  }
};

template<typename T>
Limit<T> limit(const T &item, size_t size) {
  return Limit<T>(item, size);
}

}  // namespace kosak::coding
