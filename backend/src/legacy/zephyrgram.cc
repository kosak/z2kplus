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

#include "z2kplus/backend/legacy/zephyrgram.h"

#include <string>
#include <tuple>
#include <utility>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "z2kplus/backend/shared/merge.h"
#include "kosak/coding/myjson.h"
#include "z2kplus/backend/util/misc.h"

using kosak::coding::FailFrame;
using kosak::coding::KeyMaster;
using kosak::coding::ParseContext;
using kosak::coding::streamf;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::legacy {

ZgramCore::ZgramCore() = default;

ZgramCore::ZgramCore(std::string sender, std::string signature, std::string clss,
    std::string instance, std::string body,
  RenderStyle renderStyle) :
  sender_(std::move(sender)), signature_(std::move(signature)), clss_(std::move(clss)),
  instance_(std::move(instance)), body_(std::move(body)), renderStyle_(renderStyle) {
}

ZgramCore::ZgramCore(ZgramCore &&other) noexcept = default;
ZgramCore &ZgramCore::operator=(ZgramCore &&other) noexcept = default;
ZgramCore::~ZgramCore() = default;

std::ostream &operator<<(std::ostream &s, const ZgramCore &o) {
  return streamf(s, "[%o,%o,%o,%o,%o,%o]", o.sender(), o.signature(), o.clss(), o.instance(),
    o.body(), o.renderStyle());
}

DEFINE_TYPICAL_JSON(ZgramCore, sender_, signature_, clss_, instance_, body_, renderStyle_);

Zephyrgram::Zephyrgram() = default;

Zephyrgram::Zephyrgram(ZgramId zgramId, uint64 timesecs, bool isLogged, ZgramCore zgramCore) :
  zgramId_(zgramId), timesecs_(timesecs), isLogged_(isLogged), zgramCore_(std::move(zgramCore)) {
}

Zephyrgram::Zephyrgram(Zephyrgram &&other) noexcept = default;
Zephyrgram &Zephyrgram::operator=(Zephyrgram &&other) noexcept = default;
Zephyrgram::~Zephyrgram() = default;

std::ostream &operator<<(std::ostream &o, const Zephyrgram &zg) {
  return streamf(o, "[id=%o, ts=%o, logged=%o, zgc=%o]", zg.zgramId_, zg.timesecs_, zg.isLogged_,
    zg.zgramCore_);
}

DEFINE_TYPICAL_JSON(Zephyrgram, zgramId_, timesecs_, isLogged_, zgramCore_);

PerZgramMetadataCore::PerZgramMetadataCore() = default;
PerZgramMetadataCore::PerZgramMetadataCore(reactions_t &&reactions, hashtags_t &&hashtags,
  bookmarks_t &&bookmarks, refersTo_t &&refersTo, referredFrom_t &&referredFrom,
  threads_t &&threads, edits_t &&edits, pluspluses_t &&pluspluses, watches_t &&watches) :
  reactions_(std::move(reactions)), hashtags_(std::move(hashtags)),
  bookmarks_(std::move(bookmarks)), refersTo_(std::move(refersTo)),
  referredFrom_(std::move(referredFrom)), threads_(std::move(threads)),
  edits_(std::move(edits)), pluspluses_(std::move(pluspluses)), watches_(std::move(watches)) {}
PerZgramMetadataCore::PerZgramMetadataCore(PerZgramMetadataCore &&other) noexcept = default;
PerZgramMetadataCore &PerZgramMetadataCore::operator=(PerZgramMetadataCore &&other) noexcept = default;
PerZgramMetadataCore::~PerZgramMetadataCore() = default;


std::ostream &operator<<(std::ostream &s, const PerZgramMetadataCore &o) {
  return streamf(s, "{reactions=%o"
      "\nhashtags=%o"
      "\nbookmarks=%o"
      "\nrefersTo=%o"
      "\nreferredFrom=%o"
      "\nthreadIds=%o"
      "\nedits=%o"
      "\npluspluses=%o"
      "\nwatches=%o"
      "}",
    o.reactions_, o.hashtags_, o.bookmarks_, o.refersTo_, o.referredFrom_, o.threads_, o.edits_,
    o.pluspluses_, o.watches_);
}

namespace {
constexpr auto perZgramMetadataCoreKeys = std::experimental::make_array("a", "h", "b", "r", "R", "t", "e", "p", "w");
};  // namespace

bool PerZgramMetadataCore::tryAppendJsonHelper(std::string *result, const FailFrame &ff) const {
  auto values = std::tie(reactions_, hashtags_, bookmarks_, refersTo_,
    referredFrom_, threads_, edits_, pluspluses_, watches_);
  auto present = std::experimental::make_array(
    !reactions_.empty(),
    !hashtags_.empty(),
    !bookmarks_.empty(),
    !refersTo_.empty(),
    !referredFrom_.empty(),
    !threads_.empty(),
    !edits_.empty(),
    !pluspluses_.empty(),
    !watches_.empty());
  static_assert(perZgramMetadataCoreKeys.size() == std::tuple_size_v<decltype(values)>);
  KeyMaster keyMaster(perZgramMetadataCoreKeys);
  return tryAppendJsonDictionary(keyMaster, values, present, result, ff.nest(HERE));
}

bool PerZgramMetadataCore::tryParseJsonHelper(ParseContext *ctx, const FailFrame &ff) {
  auto values = std::tie(reactions_, hashtags_, bookmarks_, refersTo_,
    referredFrom_, threads_, edits_, pluspluses_, watches_);
  KeyMaster keyMaster(perZgramMetadataCoreKeys);
  return tryParseJsonDictionary(ctx, keyMaster, &values, ff.nestWithType<PerZgramMetadataCore>(HERE));
}

PerUseridMetadataCore::PerUseridMetadataCore() = default;
PerUseridMetadataCore::PerUseridMetadataCore(zmojis_t &&zmojis, alerts_t &&alerts) :
  zmojis_(std::move(zmojis)), alerts_(std::move(alerts)) {}
PerUseridMetadataCore::PerUseridMetadataCore(PerUseridMetadataCore &&other) noexcept = default;
PerUseridMetadataCore &PerUseridMetadataCore::operator=(PerUseridMetadataCore &&other) noexcept = default;
PerUseridMetadataCore::~PerUseridMetadataCore() = default;

std::ostream &operator<<(std::ostream &s, const PerUseridMetadataCore &o) {
  return streamf(s, "{zmojis=%o, alerts=%o}", o.zmojis_, o.alerts_);
}

namespace {
constexpr auto perUseridMetadataCoreKeys = std::experimental::make_array("z", "a");
}

bool PerUseridMetadataCore::tryAppendJsonHelper(std::string *result, const FailFrame &ff) const {
  auto values = std::tie(zmojis_, alerts_);
  auto present = std::experimental::make_array(!zmojis_.empty(), !alerts_.empty());
  static_assert(perUseridMetadataCoreKeys.size() == std::tuple_size_v<decltype(values)>);
  KeyMaster keyMaster(perUseridMetadataCoreKeys);
  return tryAppendJsonDictionary(keyMaster, values, present, result, ff.nest(HERE));
}

bool PerUseridMetadataCore::tryParseJsonHelper(ParseContext *ctx, const FailFrame &ff) {
  auto values = std::tie(zmojis_, alerts_);
  static_assert(perUseridMetadataCoreKeys.size() == std::tuple_size_v<decltype(values)>);
  KeyMaster keyMaster(perUseridMetadataCoreKeys);
  return tryParseJsonDictionary(ctx, keyMaster, &values, ff.nestWithType<PerUseridMetadataCore>(HERE));
}

Metadata::Metadata() = default;
Metadata::Metadata(pzg_t &&pzg, pu_t &&pu) : pzg_(std::move(pzg)), pu_(std::move(pu)) {}
Metadata::Metadata(Metadata &&other) noexcept = default;
Metadata &Metadata::operator=(Metadata &&other) noexcept = default;
Metadata::~Metadata() = default;

namespace {
constexpr auto metadataKeys = std::experimental::make_array("z", "u");
}

bool Metadata::tryAppendJsonHelper(std::string *result, const FailFrame &ff) const {
  auto values = std::tie(pzg_, pu_);
  auto present = std::experimental::make_array(!pzg_.empty(), !pu_.empty());
  static_assert(metadataKeys.size() == std::tuple_size_v<decltype(values)>);
  KeyMaster keyMaster(metadataKeys);
  return tryAppendJsonDictionary(keyMaster, values, present, result, ff.nest(HERE));
}

bool Metadata::tryParseJsonHelper(ParseContext *ctx, const FailFrame &ff) {
  auto values = std::tie(pzg_, pu_);
  static_assert(metadataKeys.size() == std::tuple_size_v<decltype(values)>);
  KeyMaster keyMaster(metadataKeys);
  return tryParseJsonDictionary(ctx, keyMaster, &values, ff.nestWithType<Metadata>(HERE));
}

std::ostream &operator<<(std::ostream &s, const Metadata &o) {
  return streamf(s, "{pzg=%o"
      "\npu=%o"
      "}", o.pzg_, o.pu_);
}

LogRecord::LogRecord() = default;
LogRecord::LogRecord(Zephyrgram &&zgram) : payload_(std::move(zgram)) {}
LogRecord::LogRecord(Metadata &&md) : payload_(std::move(md)) {}
LogRecord::LogRecord(LogRecord &&other) noexcept = default;
LogRecord &LogRecord::operator=(LogRecord &&other) noexcept = default;
LogRecord::~LogRecord() = default;
}  // namespace z2kplus::backend::legacy
