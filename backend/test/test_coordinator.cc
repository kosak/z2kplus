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
#include <thread>
#include <variant>
#include <vector>
#include "catch/catch.hpp"
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "z2kplus/backend/communicator/communicator.h"
#include "z2kplus/backend/coordinator/coordinator.h"
#include "z2kplus/backend/coordinator/subscription.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/reverse_index/index/consolidated_index.h"
#include "z2kplus/backend/shared/protocol/message/dresponse.h"
#include "z2kplus/backend/shared/profile.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/test/util/test_util.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::ParseContext;
using kosak::coding::Unit;
using z2kplus::backend::coordinator::Coordinator;
using z2kplus::backend::server::Server;
using z2kplus::backend::coordinator::Subscription;
using z2kplus::backend::communicator::Communicator;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::reverse_index::index::ConsolidatedIndex;
using z2kplus::backend::shared::protocol::message::DRequest;
using z2kplus::backend::shared::protocol::message::DResponse;
using z2kplus::backend::shared::MetadataRecord;
using z2kplus::backend::shared::Profile;
using z2kplus::backend::shared::RenderStyle;
using z2kplus::backend::shared::SearchOrigin;
using z2kplus::backend::shared::ZgramCore;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::shared::protocol::Estimates;
using z2kplus::backend::util::BlockingQueue;
using z2kplus::backend::util::MySocket;
using z2kplus::backend::test::util::TestUtil;
namespace nsunix = kosak::coding::nsunix;
namespace drequests = z2kplus::backend::shared::protocol::message::drequests;
namespace dresponses = z2kplus::backend::shared::protocol::message::dresponses;
namespace zgMetadata = z2kplus::backend::shared::zgMetadata;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::test {

namespace {
bool tryGetPathMaster(std::shared_ptr<PathMaster> *result, const FailFrame &ff);

struct Reactor {
  Reactor();
  ~Reactor();

  void processResponses(std::vector<Coordinator::response_t> *responses);

  void operator()(dresponses::AckSubscribe &&o);
  void operator()(dresponses::AckMoreZgrams &&o);
  void operator()(dresponses::EstimatesUpdate &&o);

  template<typename T>
  void operator()(T &&item) const {
    // ignore other responses.
  }

  bool tryExpect(std::initializer_list<uint64_t> newIds, bool forBackSide, size_t front,
      size_t back, const FailFrame &ff);

  std::shared_ptr<PathMaster> pm_;
  Coordinator c_;
  std::shared_ptr<Subscription> sub_;
  bool valid_ = false;

  std::vector<ZgramId> newIds_;
  Estimates estimates_;
};
}  // namespace

TEST_CASE("coordinator: a metadata change happens", "[coordinator]") {
  FailRoot fr;
  drequests::Subscribe subReq(R"(hasreaction("👍"))", SearchOrigin(ZgramId(30)), 10, 25);

  ConsolidatedIndex ci;
  Reactor rx;
  auto profile = std::make_shared<Profile>("kosak", "Corey Kosak");
  if (!tryGetPathMaster(&rx.pm_, fr.nest(HERE)) ||
    !TestUtil::trySetupConsolidatedIndex(rx.pm_, &ci, fr.nest(HERE)) ||
    !Coordinator::tryCreate(rx.pm_, std::move(ci), &rx.c_, fr.nest(HERE))) {
    FAIL(fr);
  }
  {
    std::vector<Coordinator::response_t> responses;
    rx.c_.subscribe(std::move(profile), std::move(subReq), &responses, &rx.sub_);
    rx.processResponses(&responses);
  }

  if (!rx.valid_) {
    FAIL("Subscription failed apparently (probably a bad query)");
  }

  {
    // We post that we like 2 and 50.
    MetadataRecord mdr0(zgMetadata::Reaction(ZgramId(50), "👍", "kosak", true));
    MetadataRecord mdr1(zgMetadata::Reaction(ZgramId(2), "👍", "kosak", true));
    std::vector<MetadataRecord> metadata;
    metadata.push_back(std::move(mdr0));
    metadata.push_back(std::move(mdr1));
    drequests::PostMetadata newPost(std::move(metadata));

    std::vector<Coordinator::response_t> responses;
    rx.c_.postMetadata(rx.sub_.get(), std::move(newPost), &responses);
    rx.processResponses(&responses);
    if (!rx.tryExpect({30, 41}, true, 1, 0, fr.nest(HERE)) ||
        !rx.tryExpect({0}, false, 0, 0, fr.nest(HERE))) {
      FAIL(fr);
    }
  }
}

TEST_CASE("coordinator: post a zgram with a reply-to", "[coordinator]") {
  FailRoot fr;
  drequests::Subscribe subReq("", SearchOrigin(Unit()), 10, 25);

  ConsolidatedIndex ci;
  Reactor rx;
  auto profile = std::make_shared<Profile>("kosak", "Corey Kosak");
  if (!tryGetPathMaster(&rx.pm_, fr.nest(HERE)) ||
      !TestUtil::trySetupConsolidatedIndex(rx.pm_, &ci, fr.nest(HERE)) ||
      !Coordinator::tryCreate(rx.pm_, std::move(ci), &rx.c_, fr.nest(HERE))) {
    FAIL(fr);
  }
  {
    std::vector<Coordinator::response_t> responses;
    rx.c_.subscribe(std::move(profile), std::move(subReq), &responses, &rx.sub_);
    rx.processResponses(&responses);
  }

  if (!rx.valid_) {
    FAIL("Subscription failed apparently (probably a bad query)");
  }

  {
    // We post a reply to zgram 71
    ZgramCore zgc("appreciation.anti.t", "tpnn", RenderStyle::Default);
    drequests::PostZgrams::entry_t entry(std::move(zgc), ZgramId(71));
    drequests::PostZgrams newPost({std::move(entry)});
    std::vector<Coordinator::response_t> responses;
    auto now = std::chrono::system_clock::now();
    rx.c_.postZgrams(rx.sub_.get(), now, std::move(newPost), &responses);
    rx.processResponses(&responses);
    std::vector<zgMetadata::ZgramRefersTo> refersTo;
    rx.c_.index().getRefersToFor(ZgramId(73), &refersTo);
    REQUIRE(refersTo.size() == 1);
    const auto &r0 = refersTo[0];
    CHECK(r0.zgramId().raw() == 73);
    CHECK(r0.refersTo().raw() == 71);
  }
}

namespace {
bool tryGetPathMaster(std::shared_ptr<PathMaster> *result, const FailFrame &ff) {
  return TestUtil::tryGetPathMaster("coordinator", result, ff.nest(HERE));
}

Reactor::Reactor() = default;
Reactor::~Reactor() = default;

void Reactor::processResponses(std::vector<Coordinator::response_t> *responses) {
  for (auto &resp : *responses) {
    std::visit(*this, std::move(resp.second.payload()));
  }
}

void Reactor::operator()(dresponses::AckSubscribe &&o) {
  valid_ = o.valid();
  if (valid_) {
    estimates_ = std::move(o.estimates());
  }
}

void Reactor::operator()(dresponses::AckMoreZgrams &&o) {
  for (const auto &zg : o.zgrams()) {
    newIds_.push_back(zg->zgramId());
  }
  estimates_ = std::move(o.estimates());
}

void Reactor::operator()(dresponses::EstimatesUpdate &&o) {
  estimates_ = std::move(o.estimates());
}

bool Reactor::tryExpect(std::initializer_list<uint64_t> newIds, bool forBackSide, size_t front,
    size_t back, const FailFrame &ff) {
  std::vector<Coordinator::response_t> responses;
  while (true) {
    const auto &which = forBackSide ? estimates_.back() : estimates_.front();
    if (which.count() == 0 && which.exact()) {
      break;
    }
    drequests::GetMoreZgrams getMore(forBackSide, 1000);
    c_.getMoreZgrams(sub_.get(), std::move(getMore), &responses);
    processResponses(&responses);
    responses.clear();
  }

  std::vector<ZgramId> expectedIds;
  expectedIds.reserve(newIds.size());
  for (auto rawId : newIds) {
    expectedIds.emplace_back(rawId);
  }

  if (expectedIds != newIds_) {
    (void)ff.failf(HERE, "updates: expected %o, actual %o", expectedIds, newIds_);
  }
  if (estimates_.front().count() != front) {
    (void)ff.failf(HERE, "front: Expected count %o, got %o", front, estimates_.front().count());
  }
  if (estimates_.back().count() != back) {
    (void)ff.failf(HERE, "back: Expected count %o, got %o", back, estimates_.back().count());
  }

  newIds_.clear();
  return ff.ok();
}
}  // namespace
}  // namespace z2kplus::backend::test
