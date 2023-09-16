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

#include <chrono>
#include <string>
#include <vector>
#include "catch/catch.hpp"
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "z2kplus/backend/communicator/message_buffer.h"
#include "z2kplus/backend/coordinator/coordinator.h"
#include "z2kplus/backend/server/server.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/reverse_index/index/consolidated_index.h"
#include "z2kplus/backend/shared/protocol/message/drequest.h"
#include "z2kplus/backend/shared/protocol/message/dresponse.h"
#include "z2kplus/backend/test/util/fake_frontend.h"
#include "z2kplus/backend/test/util/test_util.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::ParseContext;
using z2kplus::backend::communicator::MessageBuffer;
using z2kplus::backend::coordinator::Coordinator;
using z2kplus::backend::server::Server;
using z2kplus::backend::communicator::Channel;
using z2kplus::backend::communicator::Communicator;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::reverse_index::index::ConsolidatedIndex;
using z2kplus::backend::shared::protocol::control::CRequest;
using z2kplus::backend::shared::protocol::control::CResponse;
using z2kplus::backend::shared::protocol::message::DRequest;
using z2kplus::backend::shared::protocol::message::DResponse;
using z2kplus::backend::shared::RenderStyle;
using z2kplus::backend::shared::ZgramCore;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::test::util::FakeFrontend;
using z2kplus::backend::util::BlockingQueue;
using z2kplus::backend::util::MySocket;
using z2kplus::backend::test::util::TestUtil;

namespace nsunix = kosak::coding::nsunix;
namespace crequests = z2kplus::backend::shared::protocol::control::crequests;
namespace cresponses = z2kplus::backend::shared::protocol::control::cresponses;
namespace drequests = z2kplus::backend::shared::protocol::message::drequests;
namespace dresponses = z2kplus::backend::shared::protocol::message::dresponses;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::test {

namespace {
bool getPathMaster(std::shared_ptr<PathMaster> *result, const FailFrame &ff) {
  return TestUtil::tryGetPathMaster("server", result, ff.nest(HERE));
}

bool verifyResponses(const std::vector<DResponse> &responses, std::initializer_list<uint64_t> expectedRaw,
    const FailFrame &ff) {
  std::vector<ZgramId> expected;
  expected.reserve(expectedRaw.size());
  for (auto raw : expectedRaw) {
    expected.emplace_back(raw);
  }

  std::vector<ZgramId> actual;
  for (const auto &resp : responses) {
    const auto *am = std::get_if<dresponses::AckMoreZgrams>(&resp.payload());
    if (am == nullptr) {
      continue;
    }
    for (const auto &zg : am->zgrams()) {
      actual.push_back(zg->zgramId());
    }
  }
  if (expected != actual) {
    return ff.failf(HERE,
        "Expected Zgrams %o\n"
        "Actual Zgrams %o", expected, actual);
  }
  return true;
}
}  // namespace

TEST_CASE("server: fire up a server", "[server]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  ConsolidatedIndex ci;
  Coordinator coordinator;
  std::shared_ptr<Server> server;
  FakeFrontend fe;
  DRequest subReq(drequests::Subscribe("", {}, 25, 10));
  std::vector<DResponse> responses;

  std::string user = "kosak";
  std::string signature = "Corey Kosak";
  auto timeout = std::chrono::seconds(50);

  if (!getPathMaster(&pm, fr.nest(HERE)) ||
    !TestUtil::trySetupConsolidatedIndex(pm, &ci, fr.nest(HERE)) ||
    !Coordinator::tryCreate(std::move(pm), std::move(ci), &coordinator, fr.nest(HERE)) ||
    !Server::tryCreate(std::move(coordinator), 0, &server, fr.nest(HERE)) ||
    !FakeFrontend::tryCreate("localhost", server->listenPort(), std::move(user), std::move(signature),
        timeout, &fe, fr.nest(HERE)) ||
    !fe.trySend(std::move(subReq), fr.nest(HERE)) ||
    !TestUtil::tryDrainZgrams(&fe, 1000, 1000, true, timeout, &responses, fr.nest(HERE))) {
    FAIL(fr);
  }

  std::initializer_list<uint64_t> expected = {
      72, 71, 70, 63, 62, 61, 60, 52,
      51, 50, 42, 41, 40, 30, 23, 22,
      21, 20, 15, 14, 13, 12, 11, 10,
      4, 3, 2, 1, 0
  };

  debug("All received responses: %o", responses);
  if (!verifyResponses(responses, expected, fr.nest(HERE))) {
    FAIL(fr);
  }
}

TEST_CASE("server: reconnect to a server", "[server]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  ConsolidatedIndex ci;
  Coordinator coordinator;
  std::shared_ptr<Server> server;
  FakeFrontend fe1;
  DRequest subReq(drequests::Subscribe("", {}, 25, 10));

  std::string user = "kosak";
  std::string signature = "Corey Kosak";
  auto timeout = std::chrono::seconds(15);

  if (!getPathMaster(&pm, fr.nest(HERE)) ||
      !TestUtil::trySetupConsolidatedIndex(pm, &ci, fr.nest(HERE)) ||
      !Coordinator::tryCreate(std::move(pm), std::move(ci), &coordinator, fr.nest(HERE)) ||
      !Server::tryCreate(std::move(coordinator), 0, &server, fr.nest(HERE)) ||
      !FakeFrontend::tryCreate("localhost", server->listenPort(), user, signature, timeout,
          &fe1, fr.nest(HERE)) ||
      !fe1.trySend(std::move(subReq), fr.nest(HERE))) {
    FAIL(fr);
  }

  fe1.startDroppingIncoming();
  DRequest getMore(drequests::GetMoreZgrams(true, 100));
  if (!fe1.trySend(std::move(getMore), fr.nest(HERE))) {
    FAIL(fr);
  }

  std::vector<DResponse> responses;
  FakeFrontend fe2;
  if (!FakeFrontend::tryAttach("localhost", server->listenPort(), std::move(user), std::move(signature),
      timeout, fe1.sessionId(), fe1.robustifier(), &fe2, fr.nest(HERE)) ||
      !TestUtil::tryDrainZgrams(&fe2, 1000, 1000, true, timeout, &responses, fr.nest(HERE))) {
    FAIL(fr);
  }

  std::initializer_list<uint64_t> expected = {
      72, 71, 70, 63, 62, 61, 60, 52,
      51, 50, 42, 41, 40, 30, 23, 22,
      21, 20, 15, 14, 13, 12, 11, 10,
      4, 3, 2, 1, 0
  };

  debug("All received responses: %o", responses);
  if (!verifyResponses(responses, expected, fr.nest(HERE))) {
    FAIL(fr);
  }
}

TEST_CASE("server: a new matching message arrives", "[server]") {
  FailRoot fr;
  std::shared_ptr<PathMaster> pm;
  ConsolidatedIndex ci;
  Coordinator coordinator;
  std::shared_ptr<Server> server;
  FakeFrontend fe;
  DRequest subReq(drequests::Subscribe("cinnabon", {}, 25, 10));

  std::string user = "kosak";
  std::string signature = "Corey Kosak";
  auto timeout = std::chrono::seconds(15);
  std::vector<DResponse> responses;

  std::initializer_list<uint64_t> expected1 = {
      12, 11, 10
  };

  if (!getPathMaster(&pm, fr.nest(HERE)) ||
      !TestUtil::trySetupConsolidatedIndex(pm, &ci, fr.nest(HERE)) ||
      !Coordinator::tryCreate(std::move(pm), std::move(ci), &coordinator, fr.nest(HERE)) ||
      !Server::tryCreate(std::move(coordinator), 0, &server, fr.nest(HERE)) ||
      !FakeFrontend::tryCreate("localhost", server->listenPort(), std::move(user), std::move(signature),
          timeout, &fe, fr.nest(HERE)) ||
      !fe.trySend(std::move(subReq), fr.nest(HERE)) ||
      !TestUtil::tryDrainZgrams(&fe, 1000, 1000, true, timeout, &responses, fr.nest(HERE)) ||
      !verifyResponses(responses, expected1, fr.nest(HERE))) {
    FAIL(fr);
  }

  using entry_t = drequests::PostZgrams::entry_t;
  std::vector<entry_t> zgrams;
  ZgramCore zgc("so hungry", "WHERE is my Cinnabon?", RenderStyle::Default);
  zgrams.push_back(entry_t(std::move(zgc), {}));

  DRequest secondRequest(drequests::PostZgrams(std::move(zgrams)));
  if (!fe.trySend(std::move(secondRequest), fr.nest(HERE)) ||
      !TestUtil::tryDrainZgrams(&fe, 1000, 1000, true, timeout, &responses, fr.nest(HERE))) {
    FAIL(fr);
  }
  std::initializer_list<uint64_t> expected2 = {
      12, 11, 10, 73
  };
  INFO("All received messages: " << responses);
  if (!verifyResponses(responses, expected2, fr.nest(HERE))) {
    FAIL(fr);
  }
}
}  // namespace z2kplus::backend::test
