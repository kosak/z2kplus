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

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <map>

namespace z2kplus::backend::shared {
class PlusPlusScanner {
public:
  typedef std::map<std::string, int64_t, std::less<>> ppDeltas_t;

  // The net of the "tag++", "tag--", "tag??", "tag~~" entries mentioned in the body.
  // tag++ has a delta of parity, tag-- has a delta of -parity. Meanwhile tag?? and tag~~ have a
  // net delta of 0. There will be an entry here even if the tags net to zero overall.
  void scan(std::string_view body, int parity, ppDeltas_t *netPlusPlusCounts);
private:
  std::string resuableString_;
};
}  // namespace z2kplus::backend::shared
