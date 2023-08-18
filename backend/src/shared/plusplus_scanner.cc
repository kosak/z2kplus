// Copyright 2023 The Z2K Plus+ Authors
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

#include "z2kplus/backend/shared/plusplus_scanner.h"

#include "z2kplus/backend/shared/magic_constants.h"

using z2kplus::backend::shared::magicConstants::plusPlusRegex;

namespace z2kplus::backend::shared {
// std::regex plusPlusRegex(R"(([A-Za-z_][A-Za-z0-9_]*)(\+\+|--|~~|\?\?))", std::regex::optimize);
// But we carve out [cC] (as in no match for C++ / C-- / C??)
// We do this as code rather than a regex because it's faster.
void PlusPlusScanner::scan(std::string_view body, int parity, ppDeltas_t *netPlusPlusCounts) {
  auto isFirstCharOfKey = [](char ch) {
    auto uch = static_cast<unsigned char>(ch);
    return
        (uch >= 'A' && uch <= 'Z') ||
            (uch >= 'a' && uch <= 'z') ||
            uch == '_' ||
            uch > 0x7f;
  };
  auto isMiddleCharOfKey = [&isFirstCharOfKey](char ch) {
    return isFirstCharOfKey(ch) || (ch >= '0' && ch <= '9');
  };
  auto isAcceptable = [](std::string_view key) {
    if (key.size() == 1 && (key[0] == 'c' || key[0] == 'C')) {
      return false;
    }
    return true;
  };
  auto operatorCharToDelta = [parity](char ch) -> std::optional<int> {
    if (ch == '+') {
      return parity;
    }
    if (ch == '-') {
      return -parity;
    }
    if (ch == '?' || ch == '~') {
      return 0;
    }
    return {};
  };

  const char *current = body.begin();
  const char *end = body.end();
  while (current != end) {
    if (!isFirstCharOfKey(*current)) {
      ++current;
      continue;
    }
    const auto *keyStart = current;
    ++current;

    // Have [A-Za-z_{non-ascii unicode}]
    while (current != end) {
      auto ch = *current;
      if (isMiddleCharOfKey(ch)) {
        ++current;
        continue;
      }
      // Have [A-Za-z_][A-Za-z0-9_]*
      std::string_view potentialKey(keyStart, current - keyStart);
      if (potentialKey.size() > magicConstants::maxPlusPlusKeySize) {
        break;
      }
      auto delta = operatorCharToDelta(ch);
      if (!delta.has_value()) {
        break;
      }
      ++current;
      // Do I have another char and is it the doubled operator?
      if (current == end || *current != ch) {
        break;
      }
      ++current;

      // carve out [cC]++ / -- / etc
      if (!isAcceptable(potentialKey)) {
        break;
      }

      // Obsession: try to avoid a string allocation
      resuableString_ = potentialKey;
      auto ip = netPlusPlusCounts->try_emplace(std::move(resuableString_), 0).first;
      ip->second += *delta;
      break;
    }
  }
}
}  // namespace z2kplus::backend::shared
