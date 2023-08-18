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

#include "kosak/coding/dumping.h"

#include <iomanip>
#include <string_view>

#include "kosak/coding/coding.h"
#include "kosak/coding/mystream.h"

namespace kosak::coding {

std::ostream &operator<<(std::ostream &s, const Hexer &h) {
  MyOstringStream ss;
  ss << std::hex << std::uppercase << std::setfill('0');
  if (h.width_ != 0) {
    ss << std::setw((int)h.width_);
  }
  ss << h.value_;
  return s << ss.str();
}
}  // namespace kosak::coding
