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

#include <string>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/myjson.h"

namespace z2kplus::backend::shared {

class Profile {
  typedef kosak::coding::FailFrame FailFrame;
  typedef kosak::coding::ParseContext ParseContext;

public:
  Profile();
  Profile(std::string userId, std::string signature);
  DECLARE_COPY_AND_ASSIGN(Profile);
  DECLARE_MOVE_COPY_AND_ASSIGN(Profile);
  ~Profile();

  const std::string &userId() const { return userId_; }
  const std::string &signature() const { return signature_; }

private:
  std::string userId_;
  std::string signature_;

  friend std::ostream &operator<<(std::ostream &s, const Profile &o);
  friend bool tryAppendJson(const Profile &o, std::string *result,
    const FailFrame &ff);
  friend bool tryParseJson(ParseContext *ctx, Profile *res, const FailFrame &ff);
};

}  // namespace z2kplus::backend::shared
