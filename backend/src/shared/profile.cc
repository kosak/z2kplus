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

#include "kosak/coding/failures.h"
#include "z2kplus/backend/shared/profile.h"
#include <string>

using kosak::coding::FailFrame;
using kosak::coding::ParseContext;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::shared {

Profile::Profile() = default;
Profile::Profile(std::string userId, std::string signature) : userId_(std::move(userId)),
  signature_(std::move(signature)) {}
Profile::Profile(const Profile &) = default;
Profile &Profile::operator=(const Profile &) = default;
Profile::Profile(Profile &&other) noexcept = default;
Profile &Profile::operator=(Profile &&other) noexcept = default;
Profile::~Profile() = default;

std::ostream &operator<<(std::ostream &s, const Profile &o) {
  return s << "BackendGoodbye()";
}

bool tryAppendJson(const Profile &o, std::string *result, const FailFrame &ff) {
  using kosak::coding::tryAppendJson;
  auto temp = std::tie(o.userId_, o.signature_);
  return tryAppendJson(temp, result, ff.nest(HERE));
}

bool tryParseJson(ParseContext *ctx, Profile *res, const FailFrame &ff) {
  auto &o = *res;
  auto temp = std::tie(o.userId_, o.signature_);
  return tryParseJson(ctx, &temp, ff.nest(HERE));
}

}  // namespace z2kplus::backend::shared
