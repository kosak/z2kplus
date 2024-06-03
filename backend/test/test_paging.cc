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

#include <sys/stat.h>
#include <cstdint>
#include <experimental/array>
#include <memory>
#include <string>
#include <vector>
#include "catch/catch.hpp"
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "z2kplus/backend/coordinator/coordinator.h"
#include "z2kplus/backend/coordinator/subscription.h"
#include "z2kplus/backend/factories/log_parser.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/reverse_index/index/consolidated_index.h"
#include "z2kplus/backend/shared/profile.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/shared/protocol/message/drequest.h"
#include "z2kplus/backend/test/util/test_util.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::makeReservedVector;
using z2kplus::backend::coordinator::Coordinator;
using z2kplus::backend::coordinator::Subscription;
using z2kplus::backend::factories::LogParser;
using z2kplus::backend::files::CompressedFileKey;
using z2kplus::backend::files::LogLocation;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::reverse_index::FieldMask;
using z2kplus::backend::reverse_index::WordInfo;
using z2kplus::backend::reverse_index::index::ConsolidatedIndex;
using z2kplus::backend::reverse_index::iterators::WordIterator;
using z2kplus::backend::reverse_index::iterators::ZgramIterator;
using z2kplus::backend::shared::LogRecord;
using z2kplus::backend::shared::Profile;
using z2kplus::backend::shared::SearchOrigin;
using z2kplus::backend::shared::Zephyrgram;
using z2kplus::backend::shared::ZgramCore;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::shared::protocol::Estimates;
using z2kplus::backend::shared::protocol::message::DResponse;
using z2kplus::backend::test::util::TestUtil;

#define HERE KOSAK_CODING_HERE

namespace drequests = z2kplus::backend::shared::protocol::message::drequests;
namespace dresponses = z2kplus::backend::shared::protocol::message::dresponses;

namespace z2kplus::backend::test {
namespace {
bool pageTest(ZgramId start, std::vector<ZgramCore> newRecords,
  const std::vector<uint64_t> &expectedStatic, const std::vector<uint64_t> &expectedDynamic,
  const FailFrame &ff);
}  // namespace

// Page through sender:kosak starting at (0,0)
TEST_CASE("paging: sender:kosak forward starting at (0+0)", "[paging]") {
  FailRoot fr;
  ZgramId start(0);
  std::vector<uint64_t> expectedStatic = { 0, 1, 3, 10, 11, 12, 13, 14, 15, 20, 22, 23, 30, 51, 60, 61, 62, 63, 71, 72 };
  std::vector<uint64_t> expectedDynamic;
  if (!pageTest(start, {}, expectedStatic, expectedDynamic, fr.nest(HERE))) {
    FAIL(fr);
  }
}

// Page through sender:kosak starting at (4,0), paging in both directions
TEST_CASE("paging: sender:kosak both ways starting at (4+0)", "[paging]") {
  FailRoot fr;
  ZgramId start(13);
  std::vector<uint64_t> expectedStatic = {13, 14, 15, 12, 11, 10, 20, 22, 23, 3, 1, 0, 30, 51, 60, 61, 62, 63, 71, 72};
  std::vector<uint64_t> expectedDynamic;
  if (!pageTest(start, {}, expectedStatic, expectedDynamic, fr.nest(HERE))) {
    FAIL(fr);
  }
}

// Page through sender:kosak starting at (4,0), and then push through some additional zgrams
// (confirming that the deferred logic works).
TEST_CASE("paging: plus deferred", "[paging]") {
  static const char additional[] =
    R"(["kosak.STAT","No one will miss me when I'm gone.","d"])" "\n";

  ZgramId start(13);
  std::vector<uint64_t> expectedStatic = {13, 14, 15, 12, 11, 10, 20, 22, 23, 3, 1, 0, 30, 51, 60, 61, 62, 63, 71, 72};
  std::vector<uint64_t> expectedDynamic = {73};
  FailRoot fr;
  std::vector<ZgramCore> newZgrams;
  if (!TestUtil::tryParseDynamicZgrams(additional, &newZgrams, fr.nest(HERE)) ||
      !pageTest(start, std::move(newZgrams), expectedStatic, expectedDynamic, fr.nest(HERE))) {
    FAIL(fr);
  }
}

namespace {
bool tryGetPathMaster(std::shared_ptr<PathMaster> *result, const FailFrame &ff) {
  return TestUtil::tryGetPathMaster("paging", result, ff.nest(HERE));
}

struct Pager {
  Pager();
  ~Pager();

  bool tryDrain(std::vector<Coordinator::response_t> *responses, const FailFrame &ff);
  bool tryCompareExpected(const char *what, const std::vector<uint64_t> &expected, const FailFrame &ff);

  bool tryProcessResponses(std::vector<Coordinator::response_t> *responses, const FailFrame &ff);

  void operator()(dresponses::AckSyntaxCheck &&) {}
  void operator()(dresponses::AckSubscribe &&o);
  void operator()(dresponses::AckMoreZgrams &&o);
  void operator()(dresponses::EstimatesUpdate &&o);
  void operator()(dresponses::MetadataUpdate &&) {}
  void operator()(dresponses::AckSpecificZgrams &&) {}
  void operator()(dresponses::PlusPlusUpdate &&) {}
  void operator()(dresponses::AckPing &&) {}
  void operator()(dresponses::GeneralError &&o);

  std::shared_ptr<PathMaster> pm_;
  Coordinator c_;
  std::shared_ptr<Subscription> sub_;
  bool valid_ = true;

  std::vector<ZgramId> newIds_;
  Estimates estimates_;
};

bool pageTest(ZgramId start, std::vector<ZgramCore> newRecords,
    const std::vector<uint64_t> &expectedStatic, const std::vector<uint64_t> &expectedDynamic,
    const FailFrame &ff) {
  auto profile = std::make_shared<Profile>("kosak", "Corey Kosak");
  const size_t pageSize = 3;
  const size_t queryMargin = 5;
  Pager p;
  ConsolidatedIndex ci;
  drequests::Subscribe subReq("sender: kosak", SearchOrigin(start), pageSize, queryMargin);
  if (!tryGetPathMaster(&p.pm_, ff.nest(HERE)) ||
      !TestUtil::trySetupConsolidatedIndex(p.pm_, &ci, ff.nest(HERE)) ||
      !Coordinator::tryCreate(p.pm_, std::move(ci), &p.c_, ff.nest(HERE))) {
    return false;
  }
  {
    std::vector<Coordinator::response_t> responses;
    p.c_.subscribe(std::move(profile), std::move(subReq), &responses, &p.sub_);
    if (!p.tryDrain(&responses, ff.nest(HERE)) ||
        !p.tryCompareExpected("static", expectedStatic, ff.nest(HERE))) {
      return false;
    }
  }

  p.newIds_.clear();

  using entry_t = drequests::PostZgrams::entry_t;
  auto recordsWithRefers = makeReservedVector<entry_t>(newRecords.size());
  for (auto &rec : newRecords) {
    recordsWithRefers.push_back(entry_t(std::move(rec), {}));
  }

  drequests::PostZgrams post(std::move(recordsWithRefers));
  auto now = std::chrono::system_clock::now();
  std::vector<Coordinator::response_t> responses;
  p.c_.postZgrams(p.sub_.get(), now, std::move(post), &responses);
  if (!p.tryDrain(&responses, ff.nest(HERE)) ||
      !p.tryCompareExpected("dynamic", expectedDynamic, ff.nest(HERE))) {
    return false;
  }

  return true;
}

Pager::Pager() = default;
Pager::~Pager() = default;

bool Pager::tryProcessResponses(std::vector<Coordinator::response_t> *responses, const FailFrame &ff) {
  for (auto &resp : *responses) {
    std::visit(*this, std::move(resp.second.payload()));
    if (!valid_) {
      return ff.fail(HERE, "Some kind of error. Oops");
    }
  }
  return true;
}

void Pager::operator()(dresponses::AckSubscribe &&o) {
  if (o.valid()) {
    estimates_ = std::move(o.estimates());
  } else {
    valid_ = false;
  }
}

void Pager::operator()(dresponses::AckMoreZgrams &&o) {
  for (const auto &zg : o.zgrams()) {
    newIds_.push_back(zg->zgramId());
  }
  estimates_ = std::move(o.estimates());
}

void Pager::operator()(dresponses::EstimatesUpdate &&o) {
  debug("Got EstimatesUpdate%o", o);
  estimates_ = std::move(o.estimates());
}

void Pager::operator()(dresponses::GeneralError &&/*o*/) {
  valid_ = false;
}

bool Pager::tryDrain(std::vector<Coordinator::response_t> *responses, const FailFrame &ff) {
  while (true) {
    bool didSomething = false;
    if (!tryProcessResponses(responses, ff.nest(HERE))) {
      return false;
    }
    responses->clear();
    if (estimates_.back().count() != 0) {
      drequests::GetMoreZgrams getMore(true, 3);
      c_.getMoreZgrams(sub_.get(), std::move(getMore), responses);
      didSomething = true;
    }
    if (estimates_.front().count() != 0) {
      drequests::GetMoreZgrams getMore(false, 3);
      c_.getMoreZgrams(sub_.get(), std::move(getMore), responses);
      didSomething = true;
    }

    if (!didSomething) {
      return true;
    }
  }
}

bool Pager::tryCompareExpected(const char *what, const std::vector<uint64_t> &expected,
    const FailFrame &ff) {
  std::vector<ZgramId> expectedIds;
  expectedIds.reserve(expected.size());
  for (auto rawId : expected) {
    expectedIds.emplace_back(rawId);
  }
  if (expectedIds != newIds_) {
    return ff.failf(HERE, "%o: expected %o, actual %o", what, expectedIds, newIds_);
  }
  return true;
}
}  // namespace
}  // namespace z2kplus::backend::test
