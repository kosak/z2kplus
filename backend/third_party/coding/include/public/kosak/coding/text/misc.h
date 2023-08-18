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
#include <optional>
#include <string_view>
#include "kosak/coding/delegate.h"
#include "kosak/coding/failures.h"

namespace kosak::coding::text {
std::string_view trim(std::string_view s);
bool startsWith(std::string_view s, std::string_view prefix);

class Splitter {
  typedef kosak::coding::FailFrame FailFrame;

  Splitter(std::optional<std::string_view> text, char splitChar) : text_(text), splitChar_(splitChar) {}
public:
  static Splitter of(std::string_view text, char splitChar) {
    return {text, splitChar};
  }
  static Splitter ofRecords(std::string_view text, char recordDelimiter);
  ~Splitter() = default;

  bool moveNext(std::string_view *result);
  bool empty() const { return !text_.has_value(); }

  bool tryMoveNext(std::string_view *result, const FailFrame &ff);
  bool tryConfirmEmpty(const FailFrame &ff) const;

  const std::optional<std::string_view> &text() const { return text_; }

private:
  std::optional<std::string_view> text_;
  char splitChar_ = 0;
};
}  // namespace kosak::coding::text
