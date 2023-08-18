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

#include "kosak/coding/text/misc.h"

using kosak::coding::FailFrame;
#define HERE KOSAK_CODING_HERE

namespace kosak::coding::text {
Splitter Splitter::ofRecords(std::string_view text, char recordDelimiter) {
  if (text.empty()) {
    return {{}, recordDelimiter};
  }
  if (*text.rbegin() == recordDelimiter) {
    text.remove_suffix(1);
  }
  return {text, recordDelimiter};
}

bool Splitter::moveNext(std::string_view *result) {
  if (empty()) {
    return false;
  }
  auto splitPos = text_->find(splitChar_);
  if (splitPos == std::string_view::npos) {
    *result = *text_;
    text_.reset();
    return true;
  }
  *result = text_->substr(0, splitPos);
  text_->remove_prefix(splitPos + 1);
  return true;
}

bool Splitter::tryMoveNext(std::string_view *result, const FailFrame &ff) {
  if (moveNext(result)) {
    return true;
  }
  return ff.fail(HERE, "Couldn't moveNext... Splitter empty");
}

bool Splitter::tryConfirmEmpty(const FailFrame &ff) const {
  if (empty()) {
    return true;
  }
  return ff.fail(HERE, "Splitter was not empty!");
}

std::string_view trim(std::string_view s) {
  auto beginp = s.begin();
  auto endp = s.end();
  while (beginp != endp && isspace(*beginp)) {
    ++beginp;
  }
  while (beginp != endp && isspace(*std::prev(endp))) {
    --endp;
  }
  return {beginp, size_t(endp - beginp)};
}

bool startsWith(std::string_view s, const std::string_view prefix) {
  if (s.size() < prefix.size()) {
    return false;
  }
  return strncmp(s.data(), prefix.data(), prefix.size()) == 0;
}
}  // namespace kosak::coding::text
