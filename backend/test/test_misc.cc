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

#include <string>
#include "catch/catch.hpp"
#include "kosak/coding/failures.h"
#include "kosak/coding/memory/mapped_file.h"
#include "kosak/coding/unix.h"
#include "z2kplus/backend/test/util/test_util.h"
#include "z2kplus/backend/util/misc.h"

namespace z2kplus::backend::test {

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::memory::MappedFile;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::test::util::TestUtil;

#define HERE KOSAK_CODING_HERE

namespace nsunix = kosak::coding::nsunix;

namespace {
bool tryGetPathMaster(std::shared_ptr<PathMaster> *result, const FailFrame &ff);
}  // namespace

TEST_CASE("misc: Test MappedFile", "[misc]") {
  const char data[] = "I like pie\n";
  FailRoot fr;
  std::string scratchDir;
  std::shared_ptr<PathMaster> pm;
  if (!tryGetPathMaster(&pm, fr.nest(HERE))) {
    FAIL(fr);
  }
  auto fn = pm->getScratchPathFor("pie.txt");
  {
    MappedFile<char> mf;
    if (!nsunix::tryMakeFileOfSize(fn, 0600, STATIC_ARRAYSIZE(data), fr.nest(HERE)) ||
        !mf.tryMap(fn, true, fr.nest(HERE))) {
      FAIL(fr);
    }
    strncpy(mf.get(), data, STATIC_ARRAYSIZE(data));
  }
  MappedFile<char> mf;
  if (!mf.tryMap(fn, true, fr.nest(HERE))) {
    FAIL(fr);
  }
  if (strncmp(mf.get(), data, STATIC_ARRAYSIZE(data)) != 0) {
    FAIL("Data does not match!");
  }
}

namespace {
bool tryGetPathMaster(std::shared_ptr<PathMaster> *result, const FailFrame &ff) {
  return TestUtil::tryGetPathMaster("misc", result, ff.nest(HERE));
}
}  // namespace
}  // namespace z2kplus::backend::test
