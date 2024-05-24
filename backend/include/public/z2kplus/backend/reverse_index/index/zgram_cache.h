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

#include <utility>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/shared/zephyrgram.h"

namespace z2kplus::backend::reverse_index::index {
class ZgramCache {
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::files::LogLocation LogLocation;
  typedef z2kplus::backend::files::PathMaster PathMaster;
  typedef z2kplus::backend::shared::Zephyrgram Zephyrgram;
  typedef z2kplus::backend::shared::ZgramId ZgramId;

  template<typename R, typename ...Args>
  using Delegate = kosak::coding::Delegate<R, Args...>;

public:
  ZgramCache();
  explicit ZgramCache(size_t capacity);
  DECLARE_MOVE_COPY_AND_ASSIGN(ZgramCache);
  DISALLOW_COPY_AND_ASSIGN(ZgramCache);
  ~ZgramCache();

  bool tryLookupOrResolve(const PathMaster &pm,
      const std::vector<std::pair<ZgramId, LogLocation>> &locators,
      std::vector<std::shared_ptr<const Zephyrgram>> *result, const FailFrame &ff);

private:
  size_t capacity_ = 0;
  std::map<ZgramId, std::shared_ptr<const Zephyrgram>> cache_;
};
}  // namespace z2kplus::backend::reverse_index::index
