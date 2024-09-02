// Copyright 2023-2024 The Z2K Plus+ Authors
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
using z2kplus::backend::shared::protocol::Filter;
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

struct SubState {
  void operator()(dresponses::AckSubscribe &&o);
  void operator()(dresponses::FiltersUpdate &&o);

  template<typename T>
  void operator()(T &&item) const {
    // ignore other responses.
  }

  bool valid_ = false;
  std::optional<uint64_t> filterVersion_ = {};
  std::vector<Filter> filters_;
};

struct Reactor {
  Reactor();
  ~Reactor();

  void processResponses(std::vector<Coordinator::response_t> *responses);

  bool tryExpect(std::initializer_list<uint64_t> newIds, bool forBackSide, size_t front,
      size_t back, const FailFrame &ff);

  std::shared_ptr<PathMaster> pm_;
  Coordinator c_;
  std::map<std::shared_ptr<Subscription>, SubState> map_;
};
}  // namespace

TEST_CASE("filters: propose the empty filter", "[filters]") {
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

  std::shared_ptr<Subscription> sub;
  {
    std::vector<Coordinator::response_t> responses;
    rx.c_.subscribe(std::move(profile), std::move(subReq), &responses, &sub);
    rx.processResponses(&responses);
  }

  if (sub == nullptr) {
    FAIL("Subscription failed apparently (probably a bad query)");
  }

  {
    drequests::ProposeFilters pf(0, false, {});
    std::vector<Coordinator::response_t> responses;
    rx.c_.proposeFilters(sub.get(), std::move(pf), &responses);
    rx.processResponses(&responses);

    REQUIRE(1 == rx.map_.size());

    const auto &ss = rx.map_.rbegin()->second;
    REQUIRE(ss.valid_);
    REQUIRE(0 == ss.filterVersion_);
    REQUIRE(ss.filters_.empty());
  }
}

TEST_CASE("filters: filter sharing", "[filters]") {
  FailRoot fr;

  ConsolidatedIndex ci;
  Reactor rx;
  if (!tryGetPathMaster(&rx.pm_, fr.nest(HERE)) ||
      !TestUtil::trySetupConsolidatedIndex(rx.pm_, &ci, fr.nest(HERE)) ||
      !Coordinator::tryCreate(rx.pm_, std::move(ci), &rx.c_, fr.nest(HERE))) {
    FAIL(fr);
  }

  std::shared_ptr<Subscription> sub1a;
  std::shared_ptr<Subscription> sub1b;
  std::shared_ptr<Subscription> sub2;

  {
    auto profile1 = std::make_shared<Profile>("kosak", "Corey Kosak");
    auto profile2 = std::make_shared<Profile>("spock", "Spock");

    drequests::Subscribe subReq1a("", SearchOrigin(Unit()), 10, 25);
    drequests::Subscribe subReq1b("", SearchOrigin(Unit()), 10, 25);
    drequests::Subscribe subReq2("", SearchOrigin(Unit()), 10, 25);

    std::vector<Coordinator::response_t> responses;
    rx.c_.subscribe(profile1, std::move(subReq1a), &responses, &sub1a);
    rx.c_.subscribe(profile1, std::move(subReq1b), &responses, &sub1b);
    rx.c_.subscribe(profile2, std::move(subReq2), &responses, &sub2);
    rx.processResponses(&responses);
  }

  if (sub1a == nullptr || sub1b == nullptr || sub2 == nullptr) {
    FAIL("Subscription failed apparently (probably a bad query)");
  }

  {
    // kosak filters spock and all things Vulcan
    Filter filter1("spock", {}, {}, true, 999999999);
    Filter filter2({}, {}, "vulcana", true, 999999999);
    std::vector<Filter> myFilters = { std::move(filter1), std::move(filter2) };
    drequests::ProposeFilters pf(20, true, std::move(myFilters));

    std::vector<Coordinator::response_t> responses;
    rx.c_.proposeFilters(sub1a.get(), std::move(pf), &responses);
    rx.processResponses(&responses);

    auto s1ap = rx.map_.find(sub1a);
    auto s1bp = rx.map_.find(sub1b);
    auto s2 = rx.map_.find(sub2);

    if (s1ap == rx.map_.end() || s1bp == rx.map_.end() || s2 == rx.map_.end()) {
      FAIL("subs didn't get stored");
    }

    const auto &ss1a = s1ap->second;
    const auto &ss1b = s1bp->second;
    const auto &ss2 = s2->second;

    // filterVersion should be set to the current time
    REQUIRE(20 < ss1a.filterVersion_);
    REQUIRE(20 < ss1b.filterVersion_);

    REQUIRE(2 ==  ss1a.filters_.size());
    REQUIRE(2 ==  ss1b.filters_.size());

    REQUIRE(!ss2.filterVersion_.has_value());
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
    auto sp = resp.first->shared_from_this();
    auto &substate = map_[sp];
    std::visit(substate, std::move(resp.second.payload()));
  }
}

void SubState::operator()(dresponses::AckSubscribe &&o) {
  valid_ = o.valid();
}

void SubState::operator()(dresponses::FiltersUpdate &&o) {
  filterVersion_ = o.version();
  filters_ = std::move(o.filters());
}

}  // namespace
}  // namespace z2kplus::backend::test
