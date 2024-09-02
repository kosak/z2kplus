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

struct Reactor {
  Reactor();
  ~Reactor();

  void processResponses(std::vector<Coordinator::response_t> *responses);

  void operator()(dresponses::AckSubscribe &&o);
  void operator()(dresponses::FiltersUpdate &&o);

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

  std::optional<uint64_t> filterVersion_ = 0;
  std::vector<Filter> filters_;
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
  {
    std::vector<Coordinator::response_t> responses;
    rx.c_.subscribe(std::move(profile), std::move(subReq), &responses, &rx.sub_);
    rx.processResponses(&responses);
  }

  if (!rx.valid_) {
    FAIL("Subscription failed apparently (probably a bad query)");
  }

  {
    drequests::ProposeFilters pf(0, false, {});
    std::vector<Coordinator::response_t> responses;
    rx.c_.proposeFilters(rx.sub_.get(), std::move(pf), &responses);
    rx.processResponses(&responses);

    REQUIRE(0 == rx.filterVersion_);
    REQUIRE(rx.filters_.empty());
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
}

void Reactor::operator()(dresponses::FiltersUpdate &&o) {
  filterVersion_ = o.version();
  filters_ = std::move(o.filters());
}

}  // namespace
}  // namespace z2kplus::backend::test
