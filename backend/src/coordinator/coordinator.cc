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

#include "z2kplus/backend/coordinator/coordinator.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "kosak/coding/text/misc.h"
#include "z2kplus/backend/coordinator/subscription.h"
#include "z2kplus/backend/queryparsing/parser.h"
#include "z2kplus/backend/reverse_index/iterators/iterator_common.h"
#include "z2kplus/backend/reverse_index/types.h"
#include "z2kplus/backend/shared/magic_constants.h"
#include "z2kplus/backend/shared/plusplus_scanner.h"
#include "z2kplus/backend/shared/protocol/misc.h"
#include "z2kplus/backend/shared/protocol/message/drequest.h"
#include "z2kplus/backend/shared/protocol/message/dresponse.h"
#include "z2kplus/backend/shared/util.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/util/misc.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::makeReservedVector;
using kosak::coding::memory::MappedFile;
using kosak::coding::text::trim;
using kosak::coding::toString;
using kosak::coding::Unit;
using z2kplus::backend::coordinator::Subscription;
using z2kplus::backend::coordinator::internal::CachedFilters;
using z2kplus::backend::files::FileKey;
using z2kplus::backend::files::FileKeyKind;
using z2kplus::backend::files::LogLocation;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::reverse_index::FieldMask;
using z2kplus::backend::reverse_index::index::ConsolidatedIndex;
using z2kplus::backend::reverse_index::index::FrozenIndex;
using z2kplus::backend::reverse_index::iterators::IteratorContext;
using z2kplus::backend::reverse_index::iterators::zgramRel_t;
using z2kplus::backend::reverse_index::metadata::DynamicMetadata;
using z2kplus::backend::reverse_index::metadata::FrozenMetadata;
using z2kplus::backend::reverse_index::zgramOff_t;
using z2kplus::backend::shared::getZgramId;
using z2kplus::backend::shared::MetadataRecord;
using z2kplus::backend::shared::PlusPlusScanner;
using z2kplus::backend::shared::Profile;
using z2kplus::backend::shared::Zephyrgram;
using z2kplus::backend::shared::ZgramCore;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::shared::protocol::Estimates;
using z2kplus::backend::shared::protocol::message::DResponse;
using z2kplus::backend::util::streamf;
using z2kplus::backend::util::frozen::FrozenStringPool;
using z2kplus::backend::util::frozen::frozenStringRef_t;

typedef z2kplus::backend::coordinator::Coordinator::response_t response_t;

#define HERE KOSAK_CODING_HERE

namespace drequests = z2kplus::backend::shared::protocol::message::drequests;
namespace dresponses = z2kplus::backend::shared::protocol::message::dresponses;
namespace magicConstants = z2kplus::backend::shared::magicConstants;
namespace nsunix = kosak::coding::nsunix;
namespace parsing = z2kplus::backend::queryparsing;
namespace userMetadata = z2kplus::backend::shared::userMetadata;
namespace zgMetadata = z2kplus::backend::shared::zgMetadata;

namespace z2kplus::backend::coordinator {
namespace {
void updateEstimates(Subscription *sub, const ConsolidatedIndex &index, std::vector<response_t> *responses);
}  // namespace

bool Coordinator::tryCreate(std::shared_ptr<PathMaster> pm, ConsolidatedIndex ci, Coordinator *result,
    const FailFrame &/*ff*/) {
  *result = Coordinator(std::move(pm), std::move(ci));
  return true;
}

Coordinator::Coordinator() = default;
Coordinator::Coordinator(std::shared_ptr<PathMaster> &&pm, ConsolidatedIndex &&ci) :
  pathMaster_(std::move(pm)), index_(std::move(ci)) {}
Coordinator::Coordinator(Coordinator &&) noexcept = default;
Coordinator &Coordinator::operator=(Coordinator &&) noexcept = default;
Coordinator::~Coordinator() = default;

void Coordinator::subscribe(std::shared_ptr<Profile> profile, drequests::Subscribe &&req,
    std::vector<response_t> *responses, std::shared_ptr<Subscription> *possibleNewSub) {
  std::unique_ptr<ZgramIterator> query;
  std::shared_ptr<Subscription> sub;
  {
    FailRoot fr;
    std::string queryText(trim(req.query()));
    if (!parsing::parse(queryText, true, &query, fr.nest(HERE)) ||
        !Subscription::tryCreate(index_, std::move(profile), std::move(queryText), std::move(query), req.start(),
            req.pageSize(), req.queryMargin(), &sub, fr.nest(HERE))) {
      responses->emplace_back(nullptr, dresponses::AckSubscribe(false, toString(fr), Estimates()));
      *possibleNewSub = nullptr;
      return;
    }
  }

  auto [est, _] = sub->updateEstimates();
  subscriptions_.insert(sub);
  responses->emplace_back(sub.get(), dresponses::AckSubscribe(true, "", std::move(est)));

  const auto &userId = sub->profile()->userId();
  std::string zmojis(index_.getZmojis(userId));
  std::vector<std::shared_ptr<const MetadataRecord>> metadata;
  auto mdr = std::make_shared<const MetadataRecord>(userMetadata::Zmojis(userId, std::move(zmojis)));
  metadata.emplace_back(std::move(mdr));
  responses->emplace_back(sub.get(), dresponses::MetadataUpdate(std::move(metadata)));
  *possibleNewSub = std::move(sub);
}

void Coordinator::unsubscribe(Subscription *sub, std::vector<response_t> */*responses*/) {
  auto ip = subscriptions_.find(sub);
  if (ip != subscriptions_.end()) {
    subscriptions_.erase(ip);
  }
}

void Coordinator::checkSyntax(Subscription *sub, CheckSyntax &&cs, std::vector<response_t> *responses) {
  FailRoot fr;
  std::unique_ptr<ZgramIterator> iterator;
  auto success = parsing::parse(cs.query(), true, &iterator, fr.nest(HERE));
  auto message = success ? toString(*iterator) : toString(fr);

  dresponses::AckSyntaxCheck asc(std::move(cs.query()), success, std::move(message));
  responses->emplace_back(sub, DResponse(std::move(asc)));
}

void Coordinator::getMoreZgrams(Subscription *sub, GetMoreZgrams &&o, std::vector<response_t> *responses) {
  // Trim count so that it is no larger than pageSize.
  // Then, top up queue to trimmed(count) + margin
  auto resultSize = std::min(o.count(), sub->pageSize());
  auto targetResidualSize = resultSize + sub->queryMargin();
  auto forBackSide = o.forBackSide();
  auto *pss = forBackSide ? &sub->backStatus() : &sub->frontStatus();
  auto &residual = *pss->residual_;
  IteratorContext ctx(index_, forBackSide);

  pss->topUp(index_, sub->query(), zgramRel_t(0), targetResidualSize);

  std::vector<std::pair<ZgramId, LogLocation>> locators;
  locators.reserve(resultSize);
  while (locators.size() < resultSize && !residual.empty()) {
    const auto &zgInfo = index_.getZgramInfo(ctx.relToOff(residual.front()));
    residual.pop_front();
    locators.emplace_back(zgInfo.zgramId(), zgInfo.location());
    sub->updateDisplayed(zgInfo.zgramId());
  }

  std::vector<std::shared_ptr<const Zephyrgram>> zgrams;
  {
    FailRoot fr;
    if (!index_.zgramCache().tryLookupOrResolve(*pathMaster_, locators, &zgrams, fr.nest(HERE))) {
      dresponses::GeneralError error(toString(fr));
      responses->emplace_back(sub, std::move(error));
      return;
    }
  }
  auto [ests, _1] = sub->updateEstimates();

  // Grab all the needed metadataRecords, but while you're at it, pick out the right zgram body
  // to use.
  std::vector<MetadataRecord> metadataRecords;
  for (const auto &zgram: zgrams) {
    // This repeatedly appends to the vector.
    index_.getMetadataFor(zgram->zgramId(), &metadataRecords);
  }

  // Get the pluspluses
  std::vector<dresponses::PlusPlusUpdate::entry_t> entries;
  for (const auto &zgram: zgrams) {
    auto zgramId = zgram->zgramId();
    auto keys = index_.getPlusPlusKeys(zgramId);
    for (const auto &key: keys) {
      auto count = index_.getPlusPlusCountAfter(zgramId, key);
      entries.emplace_back(zgramId, std::string(key), count);
    }
  }

  dresponses::AckMoreZgrams ackMore(forBackSide, std::move(zgrams), std::move(ests));
  std::vector<std::shared_ptr<const MetadataRecord>> spRecords;
  spRecords.reserve(metadataRecords.size());
  for (auto &mr : metadataRecords) {
    spRecords.push_back(std::make_shared<const MetadataRecord>(std::move(mr)));
  }
  dresponses::MetadataUpdate updateMetadata(std::move(spRecords));
  responses->emplace_back(sub, std::move(ackMore));
  responses->emplace_back(sub, std::move(updateMetadata));

  if (!entries.empty()) {
    dresponses::PlusPlusUpdate ppu(std::move(entries));
    responses->emplace_back(sub, std::move(ppu));
  }
}

// Take the records in the Post message and send them to the index.
void Coordinator::postZgrams(Subscription *sub, std::chrono::system_clock::time_point now, PostZgrams &&o,
    std::vector<response_t> *responses) {
  FailRoot fr;
  if (!tryPostZgramsNoSub(*sub->profile(), now, std::move(o), responses, fr.nest(HERE))) {
    auto failure = dresponses::GeneralError(toString(fr));
    responses->emplace_back(sub, std::move(failure));
  }
}

void Coordinator::postMetadata(Subscription *sub, PostMetadata &&o, std::vector<response_t> *responses) {
  FailRoot fr;
  if (!tryPostMetadataNoSub(*sub->profile(), std::move(o), responses, fr.nest(HERE))) {
    auto failure = dresponses::GeneralError(toString(fr));
    responses->emplace_back(sub, std::move(failure));
  }
}

bool Coordinator::tryPostZgramsNoSub(const Profile &profile, std::chrono::system_clock::time_point now,
    PostZgrams &&o, std::vector<response_t> *responses, const FailFrame &ff) {
  if (o.entries().empty()) {
    return true;
  }

  ConsolidatedIndex::ppDeltaMap_t deltaMap;
  auto inReplyToMetadata = makeReservedVector<MetadataRecord>(o.entries().size());
  auto zgramCores = makeReservedVector<ZgramCore>(o.entries().size());
  auto refersToIds = makeReservedVector<std::optional<ZgramId>>(o.entries().size());
  for (auto &entry: o.entries()) {
    zgramCores.push_back(std::move(entry.first));
    refersToIds.push_back(entry.second);
  }
  std::vector<Zephyrgram> zgrams;
  if (!index_.tryAddZgrams(now, profile, std::move(zgramCores), &deltaMap, &zgrams, ff.nest(HERE))) {
    return false;
  }

  for (size_t i = 0; i != o.entries().size(); ++i) {
    const auto &zg = zgrams[i];
    const auto &refersTo = refersToIds[i];
    if (!refersTo.has_value()) {
      continue;
    }
    auto irt = zgMetadata::ZgramRefersTo(zg.zgramId(), *refersTo, true);
    inReplyToMetadata.emplace_back(std::move(irt));
  }
  notifySubscribersAboutEstimates(responses);
  notifySubscribersAboutPpChanges(deltaMap, responses);
  PostMetadata req(std::move(inReplyToMetadata));
  return tryPostMetadataNoSub(profile, std::move(req), responses, ff.nest(HERE));
}

bool Coordinator::tryPostMetadataNoSub(const Profile &profile, PostMetadata &&o, std::vector<response_t> *responses,
    const FailFrame &ff) {
  if (o.metadata().empty()) {
    return true;
  }

  ConsolidatedIndex::ppDeltaMap_t deltaMap;
  std::vector<MetadataRecord> movedMetadata;
  if (!trySanitize(profile, &o.metadata(), ff.nest(HERE)) ||
      !index_.tryAddMetadata(std::move(o.metadata()), &deltaMap, &movedMetadata, ff.nest(HERE))) {
    return false;
  }

  notifySubscribersAboutMetadata(std::move(movedMetadata), responses);
  notifySubscribersAboutPpChanges(deltaMap, responses);
  return true;
}

void Coordinator::getSpecificZgrams(Subscription *sub, GetSpecificZgrams &&o,
    std::vector<response_t> *responses) {
  auto locators = makeReservedVector<std::pair<ZgramId, LogLocation>>(o.zgramIds().size());

  for (const auto &zgramId : o.zgramIds()) {
    zgramOff_t off;
    if (!index_.tryFind(zgramId, &off)) {
      warn("Failed to find %o", zgramId);
      // for now, silently ignore.
      continue;
    }
    const auto &info = index_.getZgramInfo(off);
    locators.emplace_back(zgramId, info.location());
  }

  auto zgrams = makeReservedVector<std::shared_ptr<const Zephyrgram>>(locators.size());
  FailRoot fr;
  if (!index_.zgramCache().tryLookupOrResolve(*pathMaster_, locators, &zgrams, fr.nest(HERE))) {
    // for now, silently ignore.
    warn("Lookup failed: %o", fr);
    return;
  }

  responses->emplace_back(sub, dresponses::AckSpecificZgrams(std::move(zgrams)));
}

void Coordinator::proposeFilters(Subscription *sub, ProposeFilters &&o, std::vector<response_t> *responses) {
  const auto &userId = sub->profile()->userId();
  auto filterp = filters_.find(userId);
  if (filterp != filters_.end()) {
    if (filterp->second.version() > o.basedOnVersion()) {
      // If my filter is newer than your filter, then your lease is not valid, so
      // I'm going to reject your request and send you (the specific requestor, not everyone
      // logged in as you) updated filter information.
      dresponses::FiltersUpdate update(filterp->second.version(), filterp->second.filters());
      responses->emplace_back(sub, DResponse(std::move(update)));
      return;
    }

    if (filterp->second.version() == o.basedOnVersion() && !o.theseFiltersAreNew()) {
      // If my filter is based on the same version as your filter, and you are not
      // specifying the "theseFiltersAreNew" flag, then there is nothing new here,
      // so I won't even respond.
      return;
    }
  }

  // Either I don't have a filter, or my filter lease is older than yours, or
  // it's the same age but you've specified the "new" flag.
  uint64_t versionToUse = o.basedOnVersion();
  if (o.theseFiltersAreNew()) {
    versionToUse = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  }

  for (const auto &destSub : subscriptions_) {
    if (userId != destSub->profile()->userId()) {
      continue;
    }
    dresponses::FiltersUpdate update(versionToUse, o.filters());
    responses->emplace_back(destSub.get(), DResponse(std::move(update)));
  }

  CachedFilters cachedFilters(versionToUse, std::move(o.filters()));
  filters_.insert_or_assign(userId, std::move(cachedFilters));
}

void Coordinator::ping(Subscription *sub, Ping &&o, std::vector<response_t> *responses) {
  responses->emplace_back(sub, dresponses::AckPing(o.cookie()));
}

bool Coordinator::tryResetIndex(std::chrono::system_clock::time_point now, const FailFrame &ff) {
  ConsolidatedIndex newIndex;
  if (!ConsolidatedIndex::tryCreate(pathMaster_, now, &newIndex, ff.nest(HERE))) {
    return false;
  }

  index_ = std::move(newIndex);
  for (auto &sub : subscriptions_) {
    sub->resetIndex(index_);
  }
  return true;
}

void Coordinator::notifySubscribersAboutMetadata(std::vector<MetadataRecord> &&metadata,
    std::vector<response_t> *responses) {
  if (metadata.empty()) {
    return;
  }
  auto mdShared = makeReservedVector<std::shared_ptr<const MetadataRecord>>(metadata.size());
  for (auto &md: metadata) {
    mdShared.push_back(std::make_shared<const MetadataRecord>(std::move(md)));
  }
  for (const auto &sub : subscriptions_) {
    auto filteredMetadata = makeReservedVector<std::shared_ptr<const MetadataRecord>>(metadata.size());
    for (const auto &md : mdShared) {
      auto zgramId = getZgramId(*md);
      if (zgramId.has_value() &&
          *zgramId >= sub->displayed().first &&
          *zgramId < sub->displayed().second) {
        filteredMetadata.push_back(md);
      }

      const auto *userId = getUserId(*md);
      if (userId != nullptr && *userId == sub->profile()->userId()) {
        filteredMetadata.push_back(md);
      }
    }
    if (!filteredMetadata.empty()) {
      responses->emplace_back(sub.get(), dresponses::MetadataUpdate(std::move(filteredMetadata)));
    }
  }
}

namespace {
void gatherZgramsHelper(const ZgramId *vecBegin, const ZgramId *vecEnd,
    ZgramId beginRange, ZgramId endRange, std::vector<ZgramId> *zgsToUpdate) {
  auto beginp = std::lower_bound(vecBegin, vecEnd, beginRange);
  auto endp = std::lower_bound(vecBegin, vecEnd, endRange);
  zgsToUpdate->insert(zgsToUpdate->end(), beginp, endp);
}

void gatherFrozenZgrams(const FrozenStringPool &fsp, const FrozenMetadata::plusPluses_t &dict,
    std::string_view key, ZgramId beginRange, ZgramId endRange,
    std::vector<ZgramId> *zgs) {
  frozenStringRef_t fsr;
  if (!fsp.tryFind(key, &fsr)) {
    return;
  }
  auto vecp = dict.find(fsr);
  const auto &vec = vecp->second;
  gatherZgramsHelper(vec.data(), vec.data() + vec.size(), beginRange, endRange, zgs);
}

void gatherDynamicZgrams(const DynamicMetadata::plusPluses_t &dict, std::string_view key,
    ZgramId beginRange, ZgramId endRange, std::vector<ZgramId> *zgs) {
  auto vecp = dict.find(key);
  const auto &vec = vecp->second;
  gatherZgramsHelper(vec.data(), vec.data() + vec.size(), beginRange, endRange, zgs);
}

std::vector<ZgramId> gatherZgramsToUpdate(const ConsolidatedIndex &index,
    ZgramId beginRange, ZgramId endRange, std::string_view key) {
  std::vector<ZgramId> zgs;
  const auto &fsp = index.frozenIndex().stringPool();
  const auto &frozenMetadata = index.frozenIndex().metadata();
  const auto &dynamicMetadata = index.dynamicIndex().metadata();
  gatherFrozenZgrams(fsp, frozenMetadata.plusPluses(), key, beginRange, endRange, &zgs);
  gatherFrozenZgrams(fsp, frozenMetadata.minusMinuses(), key, beginRange, endRange, &zgs);
  gatherDynamicZgrams(dynamicMetadata.plusPluses(), key, beginRange, endRange, &zgs);
  gatherDynamicZgrams(dynamicMetadata.minusMinuses(), key, beginRange, endRange, &zgs);
  std::sort(zgs.begin(), zgs.end());
  auto newEndp = std::unique(zgs.begin(), zgs.end());
  zgs.erase(newEndp, zgs.end());
  return zgs;
}
}  // namespace

void Coordinator::notifySubscribersAboutPpChanges(const ConsolidatedIndex::ppDeltaMap_t &deltaMap,
    std::vector<response_t> *responses) {
  std::map<std::string_view, ZgramId> keyToFirstZgramId;
  for (auto &[zgramId, inner]: deltaMap) {
    for (auto &[key, count]: inner) {
      keyToFirstZgramId.try_emplace(key, zgramId);
    }
  }

  for (const auto &sub: subscriptions_) {
    std::vector<dresponses::PlusPlusUpdate::entry_t> entries;
    const auto &disp = sub->displayed();
    for (const auto &[key, firstZgramId]: keyToFirstZgramId) {
      // For the primary zgram (the first zgram where the key change was mentioned), we send the
      // new value. This helpfully covers the case where the value doesn't exist any more (e.g.
      // "foo++" was changed to "bar++" or even "baz" (no operator).
      if (firstZgramId >= disp.first && firstZgramId < disp.second) {
        auto count = index_.getPlusPlusCountAfter(firstZgramId, key);
        entries.emplace_back(firstZgramId, key, count);
      }

      // Calculate dependent zgrams (later zgrams that mention the key)
      // I want do this more efficiently but it's making my head hurt right now, so...
      auto affectedRangeBegin = std::max(firstZgramId.next(), disp.first);
      auto affectedRangeEnd = disp.second;
      if (affectedRangeBegin >= affectedRangeEnd) {
        continue;
      }

      auto zgsToUpdate = gatherZgramsToUpdate(index_, affectedRangeBegin, affectedRangeEnd, key);

      for (auto zgramId: zgsToUpdate) {
        auto count = index_.getPlusPlusCountAfter(zgramId, key);
        entries.emplace_back(zgramId, key, count);
      }
    }
    if (!entries.empty()) {
      DResponse resp(dresponses::PlusPlusUpdate(std::move(entries)));
      responses->emplace_back(sub.get(), std::move(resp));
    }
  }
}

void Coordinator::notifySubscribersAboutEstimates(std::vector<response_t> *responses) {
  for (const auto &sub: subscriptions_) {
    updateEstimates(sub.get(), index_, responses);
  }
}

namespace {
void updateEstimates(Subscription *sub, const ConsolidatedIndex &index, std::vector<response_t> *responses) {
  // It's ok to use 0 as a lower bound here because the subscription has already done some queries
  // and has established its own lower bound.
  sub->frontStatus().topUp(index, sub->query(), zgramRel_t(0), sub->queryMargin());
  sub->backStatus().topUp(index, sub->query(), zgramRel_t(0), sub->queryMargin());
  auto [ests, changed] = sub->updateEstimates();
  if (!changed) {
    return;
  }
  dresponses::EstimatesUpdate eu(std::move(ests));
  responses->emplace_back(sub, std::move(eu));
}

enum class Disposition {Accept, Reject, Defer};

struct SanitizeAnalyzer {
  SanitizeAnalyzer(const Profile &profile, const ConsolidatedIndex &ci, size_t numItems) :
    profile_(profile), ci_(ci) {
    dispositions_.reserve(numItems);
    // Might reserve too much but that's ok.
    locators_.reserve(numItems);
  }

  void operator()(const zgMetadata::Reaction &o) {
    if (profile_.userId() != o.creator()) {
      reject();
      return;
    }
    accept();
  }

  void operator()(const zgMetadata::ZgramRevision &o) {
    defer(o.zgramId());
  }

  void operator()(const zgMetadata::ZgramRefersTo &o) {
    defer(o.zgramId());
  }

  void operator()(const userMetadata::Zmojis &/*o*/) {
    accept();
  }

  void accept() {
    dispositions_.push_back(Disposition::Accept);
  }

  void reject() {
    dispositions_.push_back(Disposition::Reject);
  }

  void defer(const ZgramId &zgramId) {
    zgramOff_t zgramOff;
    if (!ci_.tryFind(zgramId, &zgramOff)) {
      dispositions_.push_back(Disposition::Reject);
      return;
    }
    const auto &zgInfo = ci_.getZgramInfo(zgramOff);
    locators_.emplace_back(zgramId, zgInfo.location());
    dispositions_.push_back(Disposition::Defer);
  }

  const Profile &profile_;
  const ConsolidatedIndex &ci_;
  std::vector<Disposition> dispositions_;
  std::vector<std::pair<ZgramId, LogLocation>> locators_;
};
}  // namespace

bool Coordinator::trySanitize(const Profile &profile, std::vector<MetadataRecord> *records,
    const FailFrame &ff) {
  SanitizeAnalyzer sa(profile, index_, records->size());
  for (const auto &rr : *records) {
    std::visit(sa, rr.payload());
  }

  std::vector<std::shared_ptr<const Zephyrgram>> zgrams;
  if (!index_.zgramCache().tryLookupOrResolve(*pathMaster_, sa.locators_, &zgrams, ff.nest(HERE))) {
    return false;
  }

  auto destIt = records->begin();
  auto zgIter = zgrams.begin();
  auto dispIt = sa.dispositions_.begin();
  for (auto srcIt = records->begin(); srcIt != records->end(); ++srcIt) {
    const auto &disp = *dispIt++;
    if (disp == Disposition::Reject) {
      continue;
    }

    if (disp == Disposition::Defer) {
      if (profile.userId() != (*zgIter++)->sender()) {
        continue;
      }
    }

    // If we got here, either the disposition is Deferred and the userIds match, or the disposition
    // is Accept. So copy the item to its destination location (unelss it's already there)
    if (destIt != srcIt) {
      *destIt = std::move(*srcIt);
    }
    ++destIt;
  }
  records->erase(destIt, records->end());
  return true;
}

namespace internal {
CachedFilters::CachedFilters() = default;
CachedFilters::CachedFilters(uint64_t version, std::vector<Filter> filters) :
  version_(version), filters_(std::move(filters)) {}
CachedFilters::CachedFilters(CachedFilters &&) noexcept = default;
CachedFilters &CachedFilters::operator=(CachedFilters &&) noexcept = default;
CachedFilters::~CachedFilters() = default;
}
}  // namespace z2kplus::backend::server
