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

#pragma once
#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/hashtable.h"
#include "kosak/coding/myjson.h"
#include "kosak/coding/strongint.h"
#include "z2kplus/backend/util/misc.h"

namespace z2kplus::backend::shared {

// A ZgramId is a monotonically-increasing, mostly-dense (there will be holes for unlogged items
// that expire) log record identifier. Used, for example, to permanently identify a Zgram.
class ZgramId {
public:
  ZgramId() = default;
  DEFINE_COPY_AND_ASSIGN(ZgramId);
  DEFINE_MOVE_COPY_AND_ASSIGN(ZgramId);
  ~ZgramId() = default;

  explicit ZgramId(uint64_t raw) : raw_(raw) {}

  DEFINE_ALL_COMPARISON_OPERATORS(ZgramId);

  int compare(const ZgramId &other) const {
    return kosak::coding::compare(&raw_, &other.raw_);
  }

  ZgramId next() const { return ZgramId(raw_ + 1); }
  uint64_t raw() const { return raw_; }

private:
  uint64_t raw_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const ZgramId &o) {
    return s << o.raw();
  }

  DECLARE_TYPICAL_JSON(ZgramId);
};

// end, timestamp, zgramId
typedef std::variant<kosak::coding::Unit, uint64_t, ZgramId> searchOriginPayload_t;
DECLARE_VARIANT_JSON(SearchOriginPayloadHolder, searchOriginPayload_t, ("end", "ts", "zg"));

class SearchOrigin {
  typedef kosak::coding::Unit Unit;
public:
  SearchOrigin();
  explicit SearchOrigin(Unit unit);
  explicit SearchOrigin(uint64_t timestamp);
  explicit SearchOrigin(const ZgramId &zgramId);
  ~SearchOrigin();

  const searchOriginPayload_t &payload() const { return payload_; }

private:
  searchOriginPayload_t payload_;

  friend std::ostream &operator<<(std::ostream &s, const SearchOrigin &o) {
    return s << o.payload_;
  }
  DECLARE_TYPICAL_JSON(SearchOrigin);
};

enum class RenderStyle {
  Default, MarkDeepMathJax
};

DECLARE_ENUM_JSON(RenderStyleHolder, RenderStyle,
    (RenderStyle::Default, RenderStyle::MarkDeepMathJax),
    ("d", "x"));

class ZgramCore {
public:
  ZgramCore();
  ZgramCore(std::string instance, std::string body, RenderStyle renderStyle);
  DECLARE_COPY_AND_ASSIGN(ZgramCore);
  DECLARE_MOVE_COPY_AND_ASSIGN(ZgramCore);
  ~ZgramCore();

  const std::string &instance() const { return instance_; }
  const std::string &body() const { return body_; }
  RenderStyle renderStyle() const { return renderStyle_; }

private:
  /**
   * The Zephyr instance. @example "help.cheese.shared"
   */
  std::string instance_;

  /**
   * The body of the message. @example "Where can I find some good cheese?"
   */
  std::string body_;

  /**
   * How the zgram should be rendered.
   */
  RenderStyle renderStyle_ = RenderStyle::Default;

  friend std::ostream &operator<<(std::ostream &o, const ZgramCore &zgb);
  DECLARE_TYPICAL_JSON(ZgramCore);

  // hack to let the Zephygram default copy constructor access my copy constructor
  friend class Zephyrgram;
};

class Zephyrgram {
public:
  Zephyrgram();
  Zephyrgram(ZgramId zgramId, uint64_t timesecs, std::string sender, std::string signature,
      bool isLogged, ZgramCore zgramCore);
  DECLARE_MOVE_COPY_AND_ASSIGN(Zephyrgram);
  ~Zephyrgram();

  ZgramId zgramId() const { return zgramId_; }
  uint64_t timesecs() const { return timesecs_; }
  const std::string &sender() const { return sender_; }
  const std::string &signature() const { return signature_; }
  bool isLogged() const { return isLogged_; }
  const ZgramCore &zgramCore() const { return zgramCore_; }

private:
  DECLARE_COPY_AND_ASSIGN(Zephyrgram);

  // Server-assigned id of the zgram. These are assigned by the server (typically sequentially,
  // but certainly in monotonically increasing order)
  ZgramId zgramId_;
  // Server-assigned time (since the epoch). Server assures that this are always in nondecreasing
  // order (to make timesecs searches more efficient).
  uint64_t timesecs_ = 0;
  // e.g. "kosak" or "kosak.z"
  std::string sender_;
  // e.g. "Corey Kosak" or "kosak via zarchive web" or custom signature like "Lord Cinnabon".
  std::string signature_;
  // Unlogged zgrams do not get backed up and also expire after 7 days.
  bool isLogged_ = true;
  ZgramCore zgramCore_;

  friend std::ostream &operator<<(std::ostream &s, const Zephyrgram &o);
  DECLARE_TYPICAL_JSON(Zephyrgram);
};

namespace zgMetadata {
class Reaction {
public:
  Reaction();
  Reaction(ZgramId zgramId, std::string reaction, std::string creator, bool value);
  DECLARE_MOVE_COPY_AND_ASSIGN(Reaction);
  ~Reaction();

  const ZgramId &zgramId() const { return zgramId_; }

  std::string &reaction() { return reaction_; }
  const std::string &reaction() const { return reaction_; }

  std::string &creator() { return creator_; }
  const std::string &creator() const { return creator_; }

  bool value() const { return value_; }

private:
  DECLARE_COPY_AND_ASSIGN(Reaction);

  ZgramId zgramId_;
  std::string reaction_;
  std::string creator_;
  bool value_ = false;

  friend std::ostream &operator<<(std::ostream &s, const Reaction &o);
  DECLARE_TYPICAL_JSON(Reaction);
};

class ZgramRevision {
public:
  ZgramRevision();
  ZgramRevision(ZgramId zgramId, ZgramCore zgc);
  DECLARE_MOVE_COPY_AND_ASSIGN(ZgramRevision);
  ~ZgramRevision();

  ZgramId zgramId() const { return zgramId_; }
  const ZgramCore &zgc() const { return zgc_; }
  ZgramCore &zgc() { return zgc_; }

private:
  DECLARE_COPY_AND_ASSIGN(ZgramRevision);

  ZgramId zgramId_;
  ZgramCore zgc_;

  friend std::ostream &operator<<(std::ostream &s, const ZgramRevision &o);
  DECLARE_TYPICAL_JSON(ZgramRevision);
};

class ZgramRefersTo {
public:
  ZgramRefersTo();
  ZgramRefersTo(ZgramId zgramId, ZgramId refersTo, bool value);
  DECLARE_MOVE_COPY_AND_ASSIGN(ZgramRefersTo);
  ~ZgramRefersTo();

  ZgramId zgramId() const { return zgramId_; }
  ZgramId refersTo() const { return refersTo_; }
  bool value() const { return value_; }

private:
  ZgramId zgramId_;
  ZgramId refersTo_;
  bool value_ = false;

  friend std::ostream &operator<<(std::ostream &s, const ZgramRefersTo &o);
  DECLARE_TYPICAL_JSON(ZgramRefersTo);
};
}  // namespace zgMetadata

namespace userMetadata {
class Zmojis {
public:
  Zmojis();
  Zmojis(std::string userId, std::string zmojis);
  DECLARE_MOVE_COPY_AND_ASSIGN(Zmojis);
  ~Zmojis();

  std::string &userId() { return userId_; }
  const std::string &userId() const { return userId_; }

  std::string &zmojis() { return zmojis_; }
  const std::string &zmojis() const { return zmojis_; }

private:
  DECLARE_COPY_AND_ASSIGN(Zmojis);

  std::string userId_;
  std::string zmojis_;

  friend std::ostream &operator<<(std::ostream &s, const Zmojis &o);
  DECLARE_TYPICAL_JSON(Zmojis);
};

// ADL: this needs to be in the namespace of one of the variant component types
typedef std::variant<zgMetadata::Reaction, zgMetadata::ZgramRevision, zgMetadata::ZgramRefersTo, userMetadata::Zmojis>
    metadataRecordPayload_t;
DECLARE_VARIANT_JSON(MetadataRecordPayloadHolder, metadataRecordPayload_t, ("rx", "zgrev", "ref", "zmojis"));
}  // namespace userMetadata


class MetadataRecord {
public:
  MetadataRecord();
  explicit MetadataRecord(zgMetadata::Reaction &&o);
  explicit MetadataRecord(zgMetadata::ZgramRevision &&o);
  explicit MetadataRecord(zgMetadata::ZgramRefersTo &&o);
  explicit MetadataRecord(userMetadata::Zmojis &&o);
  DISALLOW_COPY_AND_ASSIGN(MetadataRecord);
  DECLARE_MOVE_COPY_AND_ASSIGN(MetadataRecord);
  ~MetadataRecord();

  userMetadata::metadataRecordPayload_t &payload() { return payload_; }
  const userMetadata::metadataRecordPayload_t &payload() const { return payload_; }

private:
  userMetadata::metadataRecordPayload_t payload_;

  DECLARE_TYPICAL_JSON(MetadataRecord);

  friend std::ostream &operator<<(std::ostream &s, const MetadataRecord &o) {
    return s << o.payload_;
  }
};

typedef std::variant<Zephyrgram, MetadataRecord> logRecordPayload_t;
DECLARE_VARIANT_JSON(LogRecordPayloadHolder, logRecordPayload_t, ("z", "m"));

class LogRecord {
public:
  LogRecord();
  explicit LogRecord(Zephyrgram &&o);
  explicit LogRecord(MetadataRecord &&o);
  DISALLOW_COPY_AND_ASSIGN(LogRecord);
  DECLARE_MOVE_COPY_AND_ASSIGN(LogRecord);
  ~LogRecord();

  logRecordPayload_t &payload() { return payload_; }
  const logRecordPayload_t &payload() const { return payload_; }

private:
  logRecordPayload_t payload_;

  DECLARE_TYPICAL_JSON(LogRecord);

  friend std::ostream &operator<<(std::ostream &s, const LogRecord &o) {
    return s << o.payload_;
  }
};

}  // namespace z2kplus::backend::shared
