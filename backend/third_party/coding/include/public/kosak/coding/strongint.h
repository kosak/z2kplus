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

#include <iostream>

#include "kosak/coding/coding.h"
#include "kosak/coding/comparers.h"
#include "kosak/coding/myjson.h"

namespace kosak::coding {
template<typename ValueType, typename Tag>
class StrongInt;

template<typename ValueType, typename Tag>
std::ostream &operator<<(std::ostream &s, StrongInt<ValueType, Tag> o);

template<typename ValueType, typename Tag>
bool tryAppendJson(const StrongInt<ValueType, Tag> &o, std::string *result,
  const kosak::coding::FailFrame &ff);

template<typename ValueType, typename Tag>
bool tryParseJson(kosak::coding::ParseContext *ctx, StrongInt<ValueType, Tag> *result,
    const kosak::coding::FailFrame &ff);

template<typename ValueType, typename Tag>
StrongInt<ValueType, Tag> &operator++(StrongInt<ValueType, Tag> &lhs) {
  ++lhs.value_;
  return lhs;
}

template<typename ValueType, typename Tag>
const StrongInt<ValueType, Tag> operator++(StrongInt<ValueType, Tag> &lhs, int) {
  StrongInt<ValueType, Tag> result = lhs;
  ++lhs.value_;
  return result;
}

template<typename ValueType, typename Tag>
class StrongInt {
  typedef kosak::coding::FailFrame FailFrame;
  typedef kosak::coding::ParseContext ParseContext;
public:
  typedef ValueType value_t;

  static StrongInt max() { return StrongInt(std::numeric_limits<ValueType>::max()); }

  StrongInt() = default;
  explicit constexpr StrongInt(ValueType value) : value_(value) {}

  template<typename U>
  StrongInt constexpr addRaw(U value) const {
    return StrongInt(raw() + value);
  }

  template<typename U>
  StrongInt constexpr subtractRaw(U value) const {
    return StrongInt(raw() - value);
  }

  int compare(StrongInt other) const {
    return kosak::coding::compare(&value_, &other.value_);
  }

  constexpr ValueType raw() const { return value_; }

private:
  ValueType value_ = ValueType();

  DEFINE_ALL_COMPARISON_OPERATORS(StrongInt);
  friend std::ostream &operator<< <>(std::ostream &s, StrongInt o);
  friend bool tryAppendJson(const StrongInt &o, std::string *result, const kosak::coding::FailFrame &ff) {
    // We do not serialize the tag over JSON. The StrongInt is meant for compile type safety, not for
    // serialization safety.
    return tryAppendJson(o.value_, result, ff.nest(KOSAK_CODING_HERE));
  }
  friend bool tryParseJson(ParseContext *ctx, StrongInt *result, const kosak::coding::FailFrame &ff) {
    return tryParseJson(ctx, &result->value_, ff.nest(KOSAK_CODING_HERE));
  }

  friend StrongInt &operator++<>(StrongInt &lhs);
  friend const StrongInt operator++<>(StrongInt &lhs, int);
};

template<typename ValueType, typename Tag>
std::ostream &operator<<(std::ostream &s, StrongInt<ValueType, Tag> o) {
  return streamf(s, "%o %o", Tag::text, o.value_);
}

template<typename ValueType, typename Tag>
StrongInt<ValueType, Tag> operator+(StrongInt<ValueType, Tag> lhs, StrongInt<ValueType, Tag> rhs) {
  return StrongInt<ValueType, Tag>(lhs.raw() + rhs.raw());
}

template<typename ValueType, typename Tag>
StrongInt<ValueType, Tag> operator-(StrongInt<ValueType, Tag> lhs, StrongInt<ValueType, Tag> rhs) {
  return StrongInt<ValueType, Tag>(lhs.raw() - rhs.raw());
}
}  // namespace kosak::coding
