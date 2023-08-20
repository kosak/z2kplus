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
#include <tuple>
#include <utility>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/myjson.h"
#include "z2kplus/backend/shared/merge.h"
#include "z2kplus/backend/shared/zephyrgram.h"

using kosak::coding::ParseContext;
using kosak::coding::streamf;
using kosak::coding::Unit;

namespace zgMetadata = z2kplus::backend::shared::zgMetadata;
namespace userMetadata = z2kplus::backend::shared::userMetadata;

namespace z2kplus::backend::shared {
DEFINE_TYPICAL_JSON(ZgramId, raw_);

SearchOrigin::SearchOrigin() = default;
SearchOrigin::SearchOrigin(Unit unit) : payload_(unit) {}
SearchOrigin::SearchOrigin(uint64_t timestamp) : payload_(timestamp) {}
SearchOrigin::SearchOrigin(const ZgramId &zgramId) : payload_(zgramId) {}
SearchOrigin::~SearchOrigin() = default;

DEFINE_TYPICAL_JSON(SearchOrigin, payload_);

ZgramCore::ZgramCore() = default;
ZgramCore::ZgramCore(std::string instance, std::string body, RenderStyle renderStyle) :
    instance_(std::move(instance)), body_(std::move(body)), renderStyle_(renderStyle) {}
ZgramCore::ZgramCore(const ZgramCore &other) = default;
ZgramCore::ZgramCore(ZgramCore &&other) noexcept = default;
ZgramCore &ZgramCore::operator=(ZgramCore &&other) noexcept = default;
ZgramCore::~ZgramCore() = default;

std::ostream &operator<<(std::ostream &s, const ZgramCore &o) {
  return streamf(s, "[%o,%o,%o]", o.instance(), o.body(), o.renderStyle());
}

DEFINE_TYPICAL_JSON(ZgramCore, instance_, body_, renderStyle_);

Zephyrgram::Zephyrgram() = default;

Zephyrgram::Zephyrgram(ZgramId zgramId, uint64_t timesecs, std::string sender, std::string signature,
    bool isLogged, ZgramCore zgramCore) :
  zgramId_(zgramId), timesecs_(timesecs), sender_(std::move(sender)), signature_(std::move(signature)),
      isLogged_(isLogged), zgramCore_(std::move(zgramCore)) {
}

Zephyrgram::Zephyrgram(const Zephyrgram &other) = default;
Zephyrgram::Zephyrgram(Zephyrgram &&other) noexcept = default;
Zephyrgram &Zephyrgram::operator=(Zephyrgram &&other) noexcept = default;
Zephyrgram::~Zephyrgram() = default;

std::ostream &operator<<(std::ostream &s, const Zephyrgram &o) {
  return streamf(s, "[%o,%o,%o,%o,%o,%o]", o.zgramId_, o.timesecs_, o.sender_, o.signature_, o.isLogged_, o.zgramCore_);
}

DEFINE_TYPICAL_JSON(Zephyrgram, zgramId_, timesecs_, sender_, signature_, isLogged_, zgramCore_);

namespace zgMetadata {
Reaction::Reaction() = default;
Reaction::Reaction(ZgramId zgramId, std::string reaction, std::string creator, bool value) :
    zgramId_(zgramId), reaction_(std::move(reaction)), creator_(std::move(creator)), value_(value) {}
Reaction::Reaction(const Reaction &other) = default;
Reaction::Reaction(Reaction &&other) noexcept = default;
Reaction &Reaction::operator=(Reaction &&other) noexcept = default;
Reaction::~Reaction() = default;

DEFINE_TYPICAL_JSON(Reaction, zgramId_, reaction_, creator_, value_);

std::ostream &operator<<(std::ostream &s, const Reaction &o) {
  return streamf(s, "[%o, %o, %o, %o]", o.zgramId_, o.reaction_, o.creator_, o.value_);
}

ZgramRevision::ZgramRevision() = default;
ZgramRevision::ZgramRevision(ZgramId zgramId, ZgramCore zgc) : zgramId_(zgramId), zgc_(std::move(zgc)) {}
ZgramRevision::ZgramRevision(ZgramRevision &&other) noexcept = default;
ZgramRevision &ZgramRevision::operator=(ZgramRevision &&other) noexcept = default;
ZgramRevision::~ZgramRevision() = default;

DEFINE_TYPICAL_JSON(ZgramRevision, zgramId_, zgc_);

std::ostream &operator<<(std::ostream &s, const ZgramRevision &o) {
  return streamf(s, "[zgId=%o, zgc=%o]", o.zgramId_, o.zgc_);
}

ZgramRefersTo::ZgramRefersTo() = default;
ZgramRefersTo::ZgramRefersTo(ZgramId zgramId, ZgramId refersTo, bool value) : zgramId_(zgramId), refersTo_(refersTo),
  value_(value) {}
ZgramRefersTo::ZgramRefersTo(ZgramRefersTo &&) noexcept = default;
ZgramRefersTo &ZgramRefersTo::operator=(ZgramRefersTo &&) noexcept = default;
ZgramRefersTo::~ZgramRefersTo() = default;

DEFINE_TYPICAL_JSON(ZgramRefersTo, zgramId_, refersTo_, value_);

std::ostream &operator<<(std::ostream &s, const ZgramRefersTo &o) {
  return streamf(s, "[zgId=%o, refersTo=%o, valid=%o]", o.zgramId_, o.refersTo_, o.value_);
}


}  // namespace zgMetadata

namespace userMetadata {
Zmojis::Zmojis() = default;
Zmojis::Zmojis(std::string userId, std::string zmojis) :
    userId_(std::move(userId)), zmojis_(std::move(zmojis)) {}
Zmojis::Zmojis(const Zmojis &other) = default;
Zmojis::Zmojis(Zmojis &&other) noexcept = default;
Zmojis &Zmojis::operator=(Zmojis &&other) noexcept = default;
Zmojis::~Zmojis() = default;

DEFINE_TYPICAL_JSON(Zmojis, userId_, zmojis_);

std::ostream &operator<<(std::ostream &s, const Zmojis &o) {
  return streamf(s, "[u=%o, zms=%o]", o.userId_, o.zmojis_);
}
}  // namespace userMetadata

MetadataRecord::MetadataRecord() = default;
MetadataRecord::MetadataRecord(zgMetadata::Reaction &&o) : payload_(std::move(o)) {}
MetadataRecord::MetadataRecord(zgMetadata::ZgramRevision &&o) : payload_(std::move(o)) {}
MetadataRecord::MetadataRecord(zgMetadata::ZgramRefersTo &&o) : payload_(std::move(o)) {}
MetadataRecord::MetadataRecord(userMetadata::Zmojis &&o) : payload_(std::move(o)) {}
MetadataRecord::MetadataRecord(MetadataRecord &&other) noexcept = default;
MetadataRecord &MetadataRecord::operator=(MetadataRecord &&other) noexcept = default;
MetadataRecord::~MetadataRecord() = default;

DEFINE_TYPICAL_JSON(MetadataRecord, payload_);

LogRecord::LogRecord() = default;
LogRecord::LogRecord(Zephyrgram &&o) : payload_(std::move(o)) {}
LogRecord::LogRecord(MetadataRecord &&o) : payload_(std::move(o)) {}
LogRecord::LogRecord(LogRecord &&other) noexcept = default;
LogRecord &LogRecord::operator=(LogRecord &&other) noexcept = default;
LogRecord::~LogRecord() = default;

DEFINE_TYPICAL_JSON(LogRecord, payload_);
}  // namespace z2kplus::backend::shared
