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
#include <cstdint>
#include <string_view>
#include <utility>

namespace kosak::coding {

class Hexer {
public:
  explicit Hexer(uintmax_t value) : value_(value) {}
  explicit Hexer(uintmax_t value, size_t width) : value_(value), width_(width) {}

private:
  uintmax_t value_;
  size_t width_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const Hexer &h);
};

//class FriendlyUtf8 {
//public:
//  explicit FriendlyUtf8(char ch) : buffer_(ch), sr_(&buffer_, 1) {}
//  explicit FriendlyUtf8(std::string_view sr) : sr_(sr) {}
//  FriendlyUtf8(FriendlyUtf8 &&other) noexcept;
//
//private:
//  char buffer_ = 0;
//  std::string_view sr_;
//
//  friend std::ostream &operator<<(std::ostream &s, const FriendlyUtf8 &o);
//};

namespace internal {

template<typename Iterator, typename Renderer>
class Dumper;

template<typename Iterator, typename Renderer>
std::ostream &operator<<(std::ostream &s, const Dumper<Iterator, Renderer> &o);

template<typename Iterator, typename Renderer>
class Dumper {
public:
  Dumper(Iterator begin, Iterator end, const char *open, const char *close, const char *separator,
      Renderer renderer) : begin_(begin), end_(end), open_(open), close_(close),
      separator_(separator), renderer_(renderer) {}

private:
  Iterator begin_;
  Iterator end_;
  const char *open_ = nullptr;
  const char *close_ = nullptr;
  const char *separator_ = nullptr;
  Renderer renderer_;

  friend std::ostream &operator<<<>(std::ostream &s, const Dumper &o);
};

template<typename Iterator, typename Renderer>
std::ostream &operator<<(std::ostream &s, const Dumper<Iterator, Renderer> &o) {
  const char *sep = "";
  s << o.open_;
  for (auto i = o.begin_; i != o.end_; ++i) {
    s << sep;
    o.renderer_(s, *i);
    sep = o.separator_;
  }
  return s << o.close_;
}
}  // namespace internal


template<typename Iterator, typename Renderer>
auto dump(Iterator begin, Iterator end, const char *open, const char *close,
    const char *separator, Renderer renderer) {
  return internal::Dumper<Iterator, Renderer>(begin, end, open, close, separator, renderer);
};

template<typename Iterator>
auto dump(Iterator begin, Iterator end, const char *open, const char *close, const char *separator) {
  auto dumpSelf = [](std::ostream &s, const auto &item) { s << item; };
  return internal::Dumper<Iterator, decltype(dumpSelf)>(begin, end, open, close, separator,
    dumpSelf);
};

template<typename Iterator>
auto dump(Iterator begin, Iterator end) {
  return dump(begin, end, "[", "]", ", ");
};

template<typename Iterator>
auto dumpDeref(Iterator begin, Iterator end, const char *open, const char *close,
  const char *separator) {
  auto cb = [](std::ostream &s, auto &&item) {
    s << *item;
  };
  return dump(begin, end, open, close, separator, cb);
}
}  // namespace kosak::coding
