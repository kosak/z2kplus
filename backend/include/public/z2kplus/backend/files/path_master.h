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

#include <regex>
#include <string>
#include <string_view>
#include "kosak/coding/coding.h"
#include "kosak/coding/delegate.h"
#include "kosak/coding/failures.h"
#include "z2kplus/backend/files/keys.h"
#include "z2kplus/backend/reverse_index/types.h"

namespace z2kplus::backend::files {
class PathMaster : public std::enable_shared_from_this<PathMaster> {
  struct Private {};
  typedef kosak::coding::FailFrame FailFrame;
  template<typename R, typename ...ARGS>
  using Delegate = kosak::coding::Delegate<R, ARGS...>;

  static const char z2kIndexName[];

public:
  static bool tryCreate(std::string root, std::shared_ptr<PathMaster> *result, const FailFrame &ff);

  PathMaster(Private, std::string loggedRoot, std::string unloggedRoot, std::string indexRoot,
      std::string scratchRoot, std::string mediaRoot);
  DISALLOW_COPY_AND_ASSIGN(PathMaster);
  DISALLOW_MOVE_COPY_AND_ASSIGN(PathMaster);
  ~PathMaster();

  std::string getPlaintextPath(const CompressedFileKey &fileKey) const;
  std::string getIndexPath() const;

  std::string getScratchIndexPath() const;
  std::string getScratchPathFor(std::string_view name) const;

  bool tryGetPlaintexts(const Delegate<bool, const ExpandedFileKey &, const FailFrame &> &cb,
      const FailFrame &ff) const;

  bool tryPublishBuild(const FailFrame &ff) const;

  const std::string &scratchRoot() const { return scratchRoot_; }

private:
  std::string loggedRoot_;
  std::string unloggedRoot_;
  std::string indexRoot_;
  std::string scratchRoot_;
  std::string mediaRoot_;
};
}  // namespace z2kplus::backend::files
