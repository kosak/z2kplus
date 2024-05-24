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

#include <clocale>
#include <ctime>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/memory/mapped_file.h"
#include "z2kplus/backend/coordinator/coordinator.h"
#include "z2kplus/backend/server/server.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/reverse_index/builder/index_builder.h"
#include "z2kplus/backend/reverse_index/index/consolidated_index.h"
#include "z2kplus/backend/reverse_index/index/frozen_index.h"
#include "z2kplus/backend/shared/magic_constants.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::memory::MappedFile;
using kosak::coding::streamf;
using z2kplus::backend::coordinator::Coordinator;
using z2kplus::backend::files::FileKeyKind;
using z2kplus::backend::files::InterFileRange;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::reverse_index::builder::IndexBuilder;
using z2kplus::backend::reverse_index::index::ConsolidatedIndex;
using z2kplus::backend::reverse_index::index::FrozenIndex;
using z2kplus::backend::reverse_index::ZgramInfo;
using z2kplus::backend::server::Server;
using z2kplus::backend::shared::Zephyrgram;

namespace nsunix = kosak::coding::nsunix;
namespace magicConstants = z2kplus::backend::shared::magicConstants;

#define HERE KOSAK_CODING_HERE

namespace {
bool tryRun(int argc, char **argv, const FailFrame &ff);
bool tryStartServer(std::shared_ptr<PathMaster> pm, std::shared_ptr<Server> *result,
    const FailFrame &ff);
}  // namespace

int main(int argc, char **argv) {
  kosak::coding::internal::Logger::elidePrefix(__FILE__, 0);

  FailRoot fr;
  if (!tryRun(argc, argv, fr.nest(HERE))) {
    streamf(std::cerr, "Failed: %o\n", fr);
    exit(1);
  }
}

namespace {
bool tryRun(int argc, char **argv, const FailFrame &ff) {
  if (argc != 2) {
    return ff.failf(HERE, "Expected 1 arguments: fileRoot");
  }

  std::shared_ptr<PathMaster> pm;
  std::shared_ptr<Server> server;
  if (!PathMaster::tryCreate(argv[1], &pm, ff.nest(HERE)) ||
      !tryStartServer(std::move(pm), &server, ff.nest(HERE))) {
    return false;
  }

  while (true) {
    std::cout << "Server is running. Enter STOP to stop.\n";

    std::string s;
    std::getline(std::cin, s);
    if (s == "STOP") {
      break;
    }
  }
  return server->tryStop(ff.nest(HERE));
}

bool tryStartServer(std::shared_ptr<PathMaster> pm, std::shared_ptr<Server> *result,
    const FailFrame &ff) {
  bool exists;
  auto indexName = pm->getIndexPath();
  if (!nsunix::tryExists(indexName, &exists, ff.nest(HERE))) {
    return false;
  }

  if (!exists) {
    // Since there's no index file, it would be safe to purge old graffiti here.
    // Otherwise it will be purged at the next index rebuild.
    if (!IndexBuilder::tryClearScratchDirectory(*pm, ff.nest(HERE)) ||
        !IndexBuilder::tryBuild(*pm,
            InterFileRange<FileKeyKind::Logged>::everything,
            InterFileRange<FileKeyKind::Unlogged>::everything,
            ff.nest(HERE)) ||
        !pm->tryPublishBuild(ff.nest(HERE))) {
      return false;
    }
  }

  ConsolidatedIndex ci;
  Coordinator coordinator;
  auto now = std::chrono::system_clock::now();
  if (!ConsolidatedIndex::tryCreate(pm, now, &ci, ff.nest(HERE)) ||
      !Coordinator::tryCreate(std::move(pm), std::move(ci), &coordinator, ff.nest(HERE)) ||
      !Server::tryCreate(std::move(coordinator), magicConstants::listenPort, result,
          ff.nest(HERE))) {
    return false;
  }
  return true;
}
}  // namespace
