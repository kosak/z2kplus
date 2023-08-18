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

#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"
#include "kosak/coding/coding.h"

int main(int argc, const char **argv) {
  kosak::coding::internal::Logger::elidePrefix(__FILE__, 1);
  Catch::Session session; // There must be exactly one instance

  int returnCode = session.applyCommandLine( argc, argv );
  if( returnCode != 0 ) // Indicates a command line error
    return returnCode;

  session.configData().runOrder = Catch::RunTests::InDeclarationOrder;

#if 0
  auto &rn = session.configData().reporterNames;
  rn.clear();
  rn.push_back("console");
#endif
  int numFailed = session.run();
  // Note that on unices only the lower 8 bits are usually used, clamping
  // the return value to 255 prevents false negative when some multiple
  // of 256 tests has failed
  return ( numFailed < 0xff ? numFailed : 0xff );
}
