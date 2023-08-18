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

#include "catch/catch.hpp"
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/reverse_index/index/consolidated_index.h"
#include "z2kplus/backend/test/util/test_util.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::reverse_index::index::ConsolidatedIndex;
using z2kplus::backend::test::util::TestUtil;

namespace zgMetadata = z2kplus::backend::shared::zgMetadata;

#define HERE KOSAK_CODING_HERE

using z2kplus::backend::shared::ZgramId;

namespace z2kplus::backend::test {
namespace {
bool getPathMaster(std::shared_ptr<PathMaster> *result, const FailFrame &ff);
}  // namespace

// body:kosak
TEST_CASE("plusplus: check counts","[plusplus]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  ConsolidatedIndex ci;
  if (!getPathMaster(&pm, fr.nest(HERE)) ||
      !TestUtil::trySetupConsolidatedIndex(pm, &ci, fr.nest(HERE))) {
    FAIL(fr);
  }

  ZgramId zgramId(72);
  CHECK(0 == ci.getPlusPlusCountAfter(ZgramId(49), "kosak"));
  CHECK(1 == ci.getPlusPlusCountAfter(ZgramId(50), "kosak"));
  CHECK(3 == ci.getPlusPlusCountAfter(ZgramId(70), "kosak"));
  CHECK(2 == ci.getPlusPlusCountAfter(ZgramId(71), "kosak"));
  CHECK(0 == ci.getPlusPlusCountAfter(zgramId, "C"));
}

namespace {
bool getPathMaster(std::shared_ptr<PathMaster> *result, const FailFrame &ff) {
  return TestUtil::tryGetPathMaster("plusplus", result, ff.nest(HERE));
}
}  // namespace
}  // namespace z2kplus::backend::test
